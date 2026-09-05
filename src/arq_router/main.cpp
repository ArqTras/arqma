// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "router_service.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/program_options.hpp>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
const char k_response[] =
    "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\nConnection: close\r\n\r\narqma-router";

bool split_host_port(const std::string& listen, std::string& host, std::uint16_t& port)
{
  const auto colon = listen.rfind(':');
  if (colon == std::string::npos || colon == 0 || colon + 1 >= listen.size())
    return false;
  host = listen.substr(0, colon);
  port = static_cast<std::uint16_t>(std::stoi(listen.substr(colon + 1)));
  return port != 0;
}
} // namespace

int main(int argc, char** argv)
{
  namespace po = boost::program_options;
  arq_router::RouterConfig cfg;
  cfg.enabled = true;
  cfg.listen = "127.0.0.1:1090";
  cfg.data_dir = "arq-router";
  po::options_description desc{"arqma-router"};
  desc.add_options()("help,h", "show help")("listen", po::value<std::string>(&cfg.listen)->default_value(cfg.listen),
                                            "host:port HTTP status")(
      "data-dir", po::value<std::string>(&cfg.data_dir)->default_value(cfg.data_dir), "router data directory");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help"))
  {
    std::cout << desc << "\nPrivacy-router process (separate from arqmad).\n";
    return 0;
  }

  arq_router::RouterService service{cfg};
  if (const auto ec = service.init())
  {
    std::cerr << "init failed: " << ec.message() << "\n";
    return 1;
  }
  if (const auto ec = service.start())
  {
    std::cerr << "start failed: " << ec.message() << "\n";
    return 1;
  }

  std::string host;
  std::uint16_t port = 0;
  if (!split_host_port(cfg.listen, host, port))
  {
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

  for (;;)
  {
    boost::system::error_code ec;
    boost::asio::ip::tcp::socket socket{io};
    acceptor.accept(socket, ec);
    if (ec)
      continue;
    boost::asio::write(socket, boost::asio::buffer(k_response, sizeof(k_response) - 1), ec);
  }
}
