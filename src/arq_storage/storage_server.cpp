// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "storage_server.h"
#include "http_io.h"
#include "arq_messaging/swarm_map.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
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
  std::string token;
  std::vector<std::string> peers;
  std::mutex mu;
  struct Stored
  {
    std::string value;
    std::uint64_t expiry = 0;
  };
  std::map<std::pair<std::string, std::string>, Stored> values;
  std::map<std::string, std::string> snodes;

  static constexpr char k_magic[4] = {'A', 'R', 'Q', '1'};
  static constexpr std::uint32_t k_max_ttl = 14 * 24 * 60 * 60;

  static std::uint64_t now_unix() { return static_cast<std::uint64_t>(std::time(nullptr)); }

  static void write_u64_le(std::ostream& out, std::uint64_t v)
  {
    for (int i = 0; i < 8; ++i)
      out.put(static_cast<char>((v >> (8 * i)) & 0xff));
  }

  static std::uint64_t read_u64_le(const std::string& raw, std::size_t off)
  {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
      v |= static_cast<std::uint64_t>(static_cast<unsigned char>(raw[off + static_cast<std::size_t>(i)])) << (8 * i);
    return v;
  }

  std::filesystem::path kv_path_on_disk(const std::string& ns, const std::string& key) const
  {
    return std::filesystem::path{data_dir} / "kv" / url_encode(ns) / url_encode(key);
  }

  std::filesystem::path snodes_path_on_disk(const std::string& pub) const
  {
    return std::filesystem::path{data_dir} / "snodes" / url_encode(pub);
  }

  void persist_kv(const std::string& ns, const std::string& key, const Stored& stored)
  {
    if (data_dir.empty())
      return;
    const auto path = kv_path_on_disk(ns, key);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (!out)
      return;
    out.write(k_magic, 4);
    write_u64_le(out, stored.expiry);
    out.write(stored.value.data(), static_cast<std::streamsize>(stored.value.size()));
  }

  void erase_kv_file(const std::string& ns, const std::string& key)
  {
    if (data_dir.empty())
      return;
    std::error_code ec;
    std::filesystem::remove(kv_path_on_disk(ns, key), ec);
  }

  static Stored decode_stored(std::string raw)
  {
    Stored stored;
    if (raw.size() >= 12 && std::memcmp(raw.data(), k_magic, 4) == 0) {
      stored.expiry = read_u64_le(raw, 4);
      stored.value = raw.substr(12);
    } else {
      stored.value = std::move(raw);
    }
    return stored;
  }

  bool expired(const Stored& stored) const { return stored.expiry != 0 && now_unix() >= stored.expiry; }

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

  void replicate(const std::string& path, const std::string& body, std::vector<std::string> extra = {})
  {
    std::vector<std::string> urls;
    {
      std::lock_guard<std::mutex> lock{mu};
      urls = peers;
    }
    urls.insert(urls.end(), extra.begin(), extra.end());
    const auto self = "http://" + format_http_authority(host, port.load());
    std::set<std::string> seen;
    seen.insert(self);
    for (auto& url : urls) {
      if (!url.empty() && url.back() == '/')
        url.pop_back();
      if (url.empty() || !seen.insert(url).second)
        continue;
      const auto ep = parse_endpoint(url);
      http_exchange(ep, "PUT", with_token(path, token), body);
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
          std::error_code size_ec;
          const auto sz = std::filesystem::file_size(key_ent.path(), size_ec);
          if (size_ec || sz > max_http_body_bytes)
            continue;
          std::ifstream in{key_ent.path(), std::ios::binary};
          std::string value((std::istreambuf_iterator<char>(in)), {});
          const auto key = url_decode(key_ent.path().filename().string());
          auto stored = decode_stored(std::move(value));
          if (expired(stored)) {
            std::filesystem::remove(key_ent.path(), ec);
            continue;
          }
          values[{ns, key}] = std::move(stored);
        }
      }
    }
    const auto sn_root = std::filesystem::path{data_dir} / "snodes";
    if (std::filesystem::exists(sn_root, ec)) {
      for (const auto& ent : std::filesystem::directory_iterator{sn_root, ec}) {
        if (!ent.is_regular_file())
          continue;
        std::error_code size_ec;
        const auto sz = std::filesystem::file_size(ent.path(), size_ec);
        if (size_ec || sz > max_http_body_bytes)
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
    if (!request_token_ok(path, token))
      return "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    {
      std::lock_guard<std::mutex> lock{mu};
      std::vector<std::pair<std::string, std::string>> drop;
      for (const auto& kv : values) {
        if (expired(kv.second))
          drop.push_back(kv.first);
      }
      for (const auto& item : drop) {
        erase_kv_file(item.first, item.second);
        values.erase(item);
      }
    }

    if (path_only == "/v1/kv") {
      const auto ns = query_get(path, "ns");
      const auto key = query_get(path, "key");
      if (ns.empty() || key.empty() || ns.size() > max_kv_name_bytes || key.size() > max_kv_name_bytes)
        return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      if (method == "PUT") {
        Stored stored;
        stored.value = body;
        std::uint32_t ttl = 0;
        const auto ttl_raw = query_get(path, "ttl");
        if (!ttl_raw.empty()) {
          ttl = static_cast<std::uint32_t>(std::strtoul(ttl_raw.c_str(), nullptr, 10));
          if (ttl > k_max_ttl)
            ttl = k_max_ttl;
          if (ttl != 0)
            stored.expiry = now_unix() + ttl;
        }
        std::vector<std::string> swarm_urls;
        {
          std::lock_guard<std::mutex> lock{mu};
          const bool inserting_new = values.find({ns, key}) == values.end();
          std::size_t ns_count = 0;
          if (inserting_new) {
            for (const auto& kv : values) {
              if (kv.first.first == ns)
                ++ns_count;
            }
          }
          if (kv_quota_exceeded(values.size(), ns_count, inserting_new))
            return "HTTP/1.1 507 Insufficient Storage\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
          values[{ns, key}] = stored;
          persist_kv(ns, key, stored);
          const auto pub = inbox_pubkey(ns);
          if (!pub.empty()) {
            const auto it = snodes.find(pub);
            if (it != snodes.end())
              swarm_urls = parse_url_lines(it->second);
          }
        }
        if (query_get(path, "replicate") != "0") {
          auto replica_path = kv_path(ns, key) + "&replicate=0";
          if (ttl != 0)
            replica_path += "&ttl=" + std::to_string(ttl);
          replicate(replica_path, body, swarm_urls);
        }
        return "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      }
      if (method == "GET") {
        std::lock_guard<std::mutex> lock{mu};
        const auto it = values.find({ns, key});
        if (it == values.end())
          return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        if (expired(it->second)) {
          erase_kv_file(ns, key);
          values.erase(it);
          return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        }
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\nContent-Length: " << it->second.value.size() << "\r\nConnection: close\r\n\r\n"
            << it->second.value;
        return oss.str();
      }
    }

    if (path_only == "/v1/snodes") {
      const auto pub = query_get(path, "pubkey");
      if (pub.empty())
        return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      if (method == "PUT") {
        std::string merged;
        std::vector<std::string> member_urls;
        {
          std::lock_guard<std::mutex> lock{mu};
          const auto it = snodes.find(pub);
          const auto existing = it == snodes.end() ? std::vector<std::string>{} : parse_url_lines(it->second);
          member_urls = merge_snode_urls(existing, parse_url_lines(body));
          merged = join_url_lines(member_urls);
          snodes[pub] = merged;
          persist_snodes(pub, merged);
        }
        if (query_get(path, "replicate") != "0")
          replicate(snodes_path(pub) + "&replicate=0", merged, member_urls);
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
      std::vector<std::pair<std::string, std::string>> drop;
      {
        std::lock_guard<std::mutex> lock{mu};
        for (const auto& kv : values) {
          if (kv.first.first != ns)
            continue;
          if (expired(kv.second)) {
            drop.push_back(kv.first);
            continue;
          }
          payload.append(kv.first.second);
          payload.push_back('\n');
        }
        for (const auto& item : drop) {
          erase_kv_file(item.first, item.second);
          values.erase(item);
        }
      }
      std::ostringstream oss;
      oss << "HTTP/1.1 200 OK\r\nContent-Length: " << payload.size() << "\r\nConnection: close\r\n\r\n" << payload;
      return oss.str();
    }

    if (path_only == "/v1/swarm" && method == "GET") {
      const auto pub = query_get(path, "pubkey");
      if (pub.empty())
        return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      std::string members;
      {
        std::lock_guard<std::mutex> lock{mu};
        const auto it = snodes.find(pub);
        if (it != snodes.end())
          members = it->second;
      }
      std::ostringstream oss;
      oss << arq_messaging::hash_pubkey_to_swarm(pub) << '\n' << members;
      const auto payload = oss.str();
      std::ostringstream http;
      http << "HTTP/1.1 200 OK\r\nContent-Length: " << payload.size() << "\r\nConnection: close\r\n\r\n" << payload;
      return http.str();
    }

    return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
  }

  void serve(boost::asio::ip::tcp::socket socket)
  {
    try {
      boost::asio::streambuf buf{max_http_header_bytes};
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
          c = static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
        if (name == "content-length")
          content_length = static_cast<std::size_t>(std::strtoul(header.c_str() + colon + 1, nullptr, 10));
      }
      if (content_length > max_http_body_bytes) {
        const char* too_large = "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        boost::asio::write(socket, boost::asio::buffer(too_large, std::strlen(too_large)), ec);
        return;
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
  return "http://" + format_http_authority(impl_->host, port());
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
  if (!base_url.empty() && base_url.back() == '/')
    base_url.pop_back();
  if (base_url.empty() || !parse_endpoint(base_url))
    return;
  std::lock_guard<std::mutex> lock{impl_->mu};
  if (std::find(impl_->peers.begin(), impl_->peers.end(), base_url) != impl_->peers.end())
    return;
  impl_->peers.push_back(std::move(base_url));
}

void StorageServer::set_token(std::string token)
{
  impl_->token = std::move(token);
}
} // namespace arq_storage
