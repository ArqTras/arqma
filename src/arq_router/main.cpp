// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "router_http.h"
#include "router_service.h"

#include "arq_messaging/identity.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/program_options.hpp>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

namespace {
bool split_host_port(const std::string& listen, std::string& host, std::uint16_t& port)
{
  const auto colon = listen.rfind(':');
  if (colon == std::string::npos || colon == 0 || colon + 1 >= listen.size())
    return false;
  host = listen.substr(0, colon);
  port = static_cast<std::uint16_t>(std::stoi(listen.substr(colon + 1)));
  return port != 0;
}

std::error_code load_or_create_identity(const std::string& data_dir, arq_messaging::Identity& id)
{
  std::error_code fs_ec;
  std::filesystem::create_directories(data_dir, fs_ec);
  const auto path = (std::filesystem::path{data_dir} / "identity").string();
  if (!arq_messaging::load_identity(path, id))
    return {};
  if (arq_messaging::generate_identity(id))
    return std::make_error_code(std::errc::io_error);
  return arq_messaging::save_identity(path, id);
}

void serve(boost::asio::ip::tcp::socket socket, const arq_messaging::Identity& hop, const std::string& storage_url)
{
  try {
    boost::asio::streambuf buf;
    boost::system::error_code ec;
    boost::asio::read_until(socket, buf, "\r\n\r\n", ec);
    if (ec)
      return;
    std::istream is{&buf};
    std::string request_line;
    std::getline(is, request_line);
    if (!request_line.empty() && request_line.back() == '\r')
      request_line.pop_back();
    std::string method;
    std::string path;
    {
      std::istringstream ls{request_line};
      std::string ver;
      ls >> method >> path >> ver;
    }
    std::size_t content_length = 0;
    std::string header;
    while (std::getline(is, header)) {
      if (!header.empty() && header.back() == '\r')
        header.pop_back();
      if (header.empty())
        break;
      const auto colon = header.find(':');
      if (colon == std::string::npos)
        continue;
      std::string name = header.substr(0, colon);
      for (char& c : name)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      if (name == "content-length")
        content_length = static_cast<std::size_t>(std::strtoul(header.c_str() + colon + 1, nullptr, 10));
    }
    std::string body(std::istreambuf_iterator<char>(is), {});
    if (content_length > body.size()) {
      std::string rest(content_length - body.size(), '\0');
      boost::asio::read(socket, boost::asio::buffer(rest), boost::asio::transfer_exactly(rest.size()), ec);
      if (!ec)
        body.append(rest);
    }
    const auto response = arq_router::handle_http(method, path, body, hop, storage_url);
    boost::asio::write(socket, boost::asio::buffer(response), ec);
  } catch (...) {
  }
}
} // namespace

int main(int argc, char** argv)
{
  namespace po = boost::program_options;
  arq_router::RouterConfig cfg;
  cfg.enabled = true;
  cfg.listen = "127.0.0.1:1090";
  cfg.data_dir = "arq-router";
  std::string storage_url;
  po::options_description desc{"arqma-router"};
  desc.add_options()("help,h", "show help")("listen", po::value<std::string>(&cfg.listen)->default_value(cfg.listen),
                                            "host:port HTTP status")(
      "data-dir", po::value<std::string>(&cfg.data_dir)->default_value(cfg.data_dir), "router data directory")(
      "storage-url", po::value<std::string>(&storage_url), "arqma-storage base URL for POST /v1/store");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc
              << "\nPrivacy-router process (separate from arqmad). POST /v1/peel peels one onion hop.\n"
                 "POST /v1/store?ns=&key=&ttl= peels then PUTs into --storage-url.\n";
    return 0;
  }

  arq_router::RouterService service{cfg};
  if (const auto ec = service.init()) {
    std::cerr << "init failed: " << ec.message() << "\n";
    return 1;
  }
  if (const auto ec = service.start()) {
    std::cerr << "start failed: " << ec.message() << "\n";
    return 1;
  }

  arq_messaging::Identity hop{};
  if (const auto ec = load_or_create_identity(cfg.data_dir, hop)) {
    std::cerr << "identity failed: " << ec.message() << "\n";
    return 1;
  }

  std::string host;
  std::uint16_t port = 0;
  if (!split_host_port(cfg.listen, host, port)) {
    std::cerr << "listen must be host:port\n";
    return 1;
  }

  boost::asio::io_context io;
  boost::asio::ip::tcp::acceptor acceptor{io};
  const auto ep = boost::asio::ip::tcp::endpoint{boost::asio::ip::make_address(host), port};
  acceptor.open(ep.protocol());
  acceptor.set_option(boost::asio::socket_base::reuse_address(true));
  acceptor.bind(ep);
  acceptor.listen();
  std::cout << "arqma-router " << service.state_name() << " on http://" << host << ":" << port << std::endl;

  for (;;) {
    boost::system::error_code ec;
    boost::asio::ip::tcp::socket socket{io};
    acceptor.accept(socket, ec);
    if (ec)
      continue;
    serve(std::move(socket), hop, storage_url);
  }
}
