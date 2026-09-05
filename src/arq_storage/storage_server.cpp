// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "storage_server.h"
#include "http_io.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace arq_storage {
struct StorageServer::Impl
{
  boost::asio::io_context io;
  boost::asio::ip::tcp::acceptor acceptor{io};
  std::thread thread;
  std::atomic<bool> running{false};
  std::atomic<std::uint16_t> port{0};
  std::string host{"127.0.0.1"};
  std::string data_dir;
  std::vector<std::string> peers;
  std::mutex mu;
  std::map<std::pair<std::string, std::string>, std::string> values;
  std::map<std::string, std::string> snodes;

  std::filesystem::path kv_path_on_disk(const std::string& ns, const std::string& key) const
  {
    return std::filesystem::path{data_dir} / "kv" / url_encode(ns) / url_encode(key);
  }

  std::filesystem::path snodes_path_on_disk(const std::string& pub) const
  {
    return std::filesystem::path{data_dir} / "snodes" / url_encode(pub);
  }

  void persist_kv(const std::string& ns, const std::string& key, const std::string& value)
  {
    if (data_dir.empty())
      return;
    const auto path = kv_path_on_disk(ns, key);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (out)
      out.write(value.data(), static_cast<std::streamsize>(value.size()));
  }

  void persist_snodes(const std::string& pub, const std::string& body)
  {
    if (data_dir.empty())
      return;
    const auto path = snodes_path_on_disk(pub);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (out)
      out.write(body.data(), static_cast<std::streamsize>(body.size()));
  }

  void replicate(const std::string& path, const std::string& body)
  {
    std::vector<std::string> urls;
    {
      std::lock_guard<std::mutex> lock{mu};
      urls = peers;
    }
    for (const auto& url : urls) {
      const auto ep = parse_endpoint(url);
      http_exchange(ep, "PUT", path, body);
    }
  }

  void load_disk()
  {
    if (data_dir.empty())
      return;
    std::error_code ec;
    const auto kv_root = std::filesystem::path{data_dir} / "kv";
    if (std::filesystem::exists(kv_root, ec)) {
      for (const auto& ns_ent : std::filesystem::directory_iterator{kv_root, ec}) {
        if (!ns_ent.is_directory())
          continue;
        const auto ns = url_decode(ns_ent.path().filename().string());
        for (const auto& key_ent : std::filesystem::directory_iterator{ns_ent.path(), ec}) {
          if (!key_ent.is_regular_file())
            continue;
          std::ifstream in{key_ent.path(), std::ios::binary};
          std::string value((std::istreambuf_iterator<char>(in)), {});
          values[{ns, url_decode(key_ent.path().filename().string())}] = std::move(value);
        }
      }
    }
    const auto sn_root = std::filesystem::path{data_dir} / "snodes";
    if (std::filesystem::exists(sn_root, ec)) {
      for (const auto& ent : std::filesystem::directory_iterator{sn_root, ec}) {
        if (!ent.is_regular_file())
          continue;
        std::ifstream in{ent.path(), std::ios::binary};
        std::string body((std::istreambuf_iterator<char>(in)), {});
        snodes[url_decode(ent.path().filename().string())] = std::move(body);
      }
    }
  }

  std::string handle(const std::string& method, const std::string& path, const std::string& body)
  {
    const auto path_only = path.substr(0, path.find('?'));
    if (method == "GET" && (path_only == "/" || path_only == "/status"))
      return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: "
             "close\r\n\r\narqma-storage";

    if (path_only == "/v1/kv") {
      const auto ns = query_get(path, "ns");
      const auto key = query_get(path, "key");
      if (ns.empty() || key.empty())
        return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      if (method == "PUT") {
        {
          std::lock_guard<std::mutex> lock{mu};
          values[{ns, key}] = body;
          persist_kv(ns, key, body);
        }
        if (query_get(path, "replicate") != "0")
          replicate(kv_path(ns, key) + "&replicate=0", body);
        return "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      }
      if (method == "GET") {
        std::lock_guard<std::mutex> lock{mu};
        const auto it = values.find({ns, key});
        if (it == values.end())
          return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\nContent-Length: " << it->second.size() << "\r\nConnection: close\r\n\r\n"
            << it->second;
        return oss.str();
      }
    }

    if (path_only == "/v1/snodes") {
      const auto pub = query_get(path, "pubkey");
      if (pub.empty())
        return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      if (method == "PUT") {
        {
          std::lock_guard<std::mutex> lock{mu};
          snodes[pub] = body;
          persist_snodes(pub, body);
        }
        if (query_get(path, "replicate") != "0")
          replicate(snodes_path(pub) + "&replicate=0", body);
        return "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      }
      if (method == "GET") {
        std::lock_guard<std::mutex> lock{mu};
        const auto it = snodes.find(pub);
        const std::string& payload = it == snodes.end() ? std::string{} : it->second;
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\nContent-Length: " << payload.size() << "\r\nConnection: close\r\n\r\n" << payload;
        return oss.str();
      }
    }

    if (path_only == "/v1/list" && method == "GET") {
      const auto ns = query_get(path, "ns");
      if (ns.empty())
        return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      std::string payload;
      {
        std::lock_guard<std::mutex> lock{mu};
        for (const auto& kv : values) {
          if (kv.first.first == ns) {
            payload.append(kv.first.second);
            payload.push_back('\n');
          }
        }
      }
      std::ostringstream oss;
      oss << "HTTP/1.1 200 OK\r\nContent-Length: " << payload.size() << "\r\nConnection: close\r\n\r\n" << payload;
      return oss.str();
    }

    return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
  }

  void serve(boost::asio::ip::tcp::socket socket)
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
      const auto response = handle(method, path, body);
      boost::asio::write(socket, boost::asio::buffer(response), ec);
    } catch (...) {
    }
  }

  void run()
  {
    while (running.load()) {
      boost::system::error_code ec;
      boost::asio::ip::tcp::socket socket{io};
      acceptor.accept(socket, ec);
      if (!running.load())
        break;
      if (ec)
        continue;
      serve(std::move(socket));
    }
  }
};

StorageServer::StorageServer() : impl_(std::make_unique<Impl>()) {}

StorageServer::~StorageServer()
{
  stop();
}

std::error_code StorageServer::listen(const std::string& host, const std::uint16_t port)
{
  stop();
  try {
    impl_->load_disk();
    const auto addr = boost::asio::ip::make_address(host);
    boost::asio::ip::tcp::endpoint ep{addr, port};
    impl_->acceptor = boost::asio::ip::tcp::acceptor{impl_->io};
    impl_->acceptor.open(ep.protocol());
    impl_->acceptor.set_option(boost::asio::socket_base::reuse_address(true));
    impl_->acceptor.bind(ep);
    impl_->acceptor.listen();
    impl_->port = impl_->acceptor.local_endpoint().port();
    impl_->host = host;
    impl_->running = true;
    impl_->thread = std::thread([this] { impl_->run(); });
    return {};
  } catch (...) {
    return std::make_error_code(std::errc::address_in_use);
  }
}

void StorageServer::stop()
{
  if (!impl_)
    return;
  impl_->running = false;
  boost::system::error_code ec;
  impl_->acceptor.cancel(ec);
  impl_->acceptor.close(ec);
  impl_->io.stop();
  if (impl_->thread.joinable())
    impl_->thread.join();
  impl_->io.restart();
  impl_->port = 0;
}

std::uint16_t StorageServer::port() const noexcept
{
  return impl_ ? impl_->port.load() : 0;
}

bool StorageServer::running() const noexcept
{
  return impl_ && impl_->running.load();
}

std::string StorageServer::base_url() const
{
  if (!running())
    return {};
  return "http://" + impl_->host + ":" + std::to_string(port());
}

void StorageServer::set_data_dir(std::string path)
{
  impl_->data_dir = std::move(path);
}

const std::string& StorageServer::data_dir() const noexcept
{
  static const std::string empty;
  return impl_ ? impl_->data_dir : empty;
}

void StorageServer::add_peer(std::string base_url)
{
  if (base_url.empty())
    return;
  std::lock_guard<std::mutex> lock{impl_->mu};
  impl_->peers.push_back(std::move(base_url));
}
} // namespace arq_storage
