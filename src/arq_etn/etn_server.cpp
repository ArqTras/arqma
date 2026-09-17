// Copyright (c) 2018 - 2026, The Arqma Network

#include "etn_server.h"
#include "etn_wallet_rpc.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <chrono>
#include <cstring>
#include <sstream>
#include <utility>
#include <vector>

namespace arq_etn {
namespace {

std::string http_ok(const std::string& body, const char* type = "application/json")
{
  return "HTTP/1.1 200 OK\r\nContent-Type: " + std::string(type) +
         "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
}

std::string http_err(int code, const std::string& msg)
{
  const std::string body = "{\"error\":\"" + msg + "\"}";
  return "HTTP/1.1 " + std::to_string(code) + " Error\r\nContent-Type: application/json\r\nContent-Length: " +
         std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
}

std::string query_get(const std::string& path, const std::string& key)
{
  const auto q = path.find('?');
  if (q == std::string::npos)
    return {};
  const std::string query = path.substr(q + 1);
  const std::string needle = key + "=";
  auto pos = query.find(needle);
  while (pos != std::string::npos) {
    if (pos == 0 || query[pos - 1] == '&') {
      auto start = pos + needle.size();
      auto end = query.find('&', start);
      return query.substr(start, end == std::string::npos ? end : end - start);
    }
    pos = query.find(needle, pos + 1);
  }
  return {};
}

std::string path_only(const std::string& path)
{
  return path.substr(0, path.find('?'));
}

std::string json_rpc_result(const std::string& id, const std::string& result_json)
{
  return "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "\"0\"" : id) + ",\"result\":" + result_json + "}";
}

std::string json_rpc_error(const std::string& id, int code, const std::string& msg)
{
  return "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "\"0\"" : id) + ",\"error\":{\"code\":" +
         std::to_string(code) + ",\"message\":\"" + msg + "\"}}";
}

std::string extract_json_string(const std::string& body, const std::string& key)
{
  const std::string needle = "\"" + key + "\":\"";
  const auto start = body.find(needle);
  if (start == std::string::npos)
    return {};
  auto i = start + needle.size();
  std::string out;
  while (i < body.size() && body[i] != '"') {
    if (body[i] == '\\' && i + 1 < body.size()) {
      out.push_back(body[i + 1]);
      i += 2;
      continue;
    }
    out.push_back(body[i++]);
  }
  return out;
}

} // namespace

struct EtNServer::Impl
{
  std::shared_ptr<EtNStore> store = std::make_shared<EtNStore>();
  std::string token;
  std::string wallet_rpc_url;
  std::string host = "127.0.0.1";
  std::atomic<std::uint16_t> port{0};
  std::atomic<bool> running{false};
  boost::asio::io_context io;
  boost::asio::ip::tcp::acceptor acceptor{io};
  std::thread thread;

  bool token_ok(const std::string& path) const
  {
    if (token.empty())
      return true;
    return query_get(path, "token") == token;
  }

  std::string status_json() const
  {
    std::ostringstream o;
    o << "{\"service\":\"arqma-etn-audit\""
      << ",\"ok\":true"
      << ",\"issuer_id\":\"" << store->issuer_id() << "\""
      << ",\"attestation_count\":" << store->attestation_count()
      << ",\"has_reserve\":" << (store->latest_reserve() ? "true" : "false")
      << ",\"wallet_rpc_configured\":" << (wallet_rpc_url.empty() ? "false" : "true") << "}";
    return o.str();
  }

  std::string refresh_reserve()
  {
    ReserveSummary r;
    r.issuer_id = store->issuer_id();
    // Liabilities stay issuer-published (default 0). Wallet balance is assets, not liabilities.
    if (const auto prev = store->latest_reserve())
      r.liability_atomic = prev->liability_atomic.empty() ? "0" : prev->liability_atomic;
    else
      r.liability_atomic = "0";
    r.as_of_height = 0;
    if (!wallet_rpc_url.empty()) {
      WalletRpcConfig cfg;
      cfg.url = wallet_rpc_url;
      const auto proof = fetch_reserve_proof_from_wallet_rpc(cfg);
      if (proof.ok) {
        r.reserve_proof_blob = proof.signature;
        r.as_of_height = proof.wallet_height;
        r.wallet_balance_atomic = proof.balance_atomic;
      } else {
        r.reserve_proof_blob = "wallet-rpc-failed:" + proof.error;
      }
    }
    if (r.reserve_proof_blob.empty()) {
      // Demo / offline stub so local ETN stacks work without wallet-rpc.
      r.reserve_proof_blob = "demo-reserve-proof";
      r.as_of_height = 1;
    }
    store->save_reserve(r);
    return reserve_to_json(r);
  }

  std::string handle_json_rpc(const std::string& body)
  {
    const auto method = extract_json_string(body, "method");
    std::string id = extract_json_string(body, "id");
    if (id.empty())
      id = "\"0\"";
    else
      id = "\"" + id + "\"";
    if (method == "etn_get_status")
      return json_rpc_result(id, status_json());
    if (method == "etn_get_reserve") {
      const auto r = store->latest_reserve();
      if (!r)
        return json_rpc_error(id, -32001, "no reserve");
      return json_rpc_result(id, reserve_to_json(*r));
    }
    if (method == "etn_refresh_reserve")
      return json_rpc_result(id, refresh_reserve());
    if (method == "etn_list_attestations") {
      const auto ids = store->list_attestation_ids();
      std::ostringstream o;
      o << "{\"ids\":[";
      for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i)
          o << ',';
        o << '"' << ids[i] << '"';
      }
      o << "]}";
      return json_rpc_result(id, o.str());
    }
    if (method == "etn_get_attestation") {
      BurnMintAttestation tmp;
      if (!attestation_from_json(body, tmp) && tmp.id.empty()) {
        // params.id
        const auto aid = extract_json_string(body, "id");
        if (aid.empty())
          return json_rpc_error(id, -32602, "missing id");
        const auto a = store->get_attestation(aid);
        if (!a)
          return json_rpc_error(id, -32004, "not found");
        return json_rpc_result(id, attestation_to_json(*a));
      }
    }
    if (method == "etn_publish_attestation") {
      BurnMintAttestation a;
      if (!attestation_from_json(body, a))
        return json_rpc_error(id, -32602, "invalid attestation");
      if (!store->put_attestation(a))
        return json_rpc_error(id, -32003, "store failed");
      return json_rpc_result(id, "{\"id\":\"" + a.id + "\"}");
    }
    return json_rpc_error(id, -32601, "method not found");
  }

  std::string handle(const std::string& method, const std::string& path, const std::string& body)
  {
    const auto p = path_only(path);
    if (method == "GET" && (p == "/" || p == "/status"))
      return http_ok("{\"service\":\"arqma-etn-audit\",\"ok\":true}");
    if (!token_ok(path) && p != "/status" && p != "/")
      return http_err(401, "unauthorized");
    if (method == "GET" && p == "/v1/etn/status")
      return http_ok(status_json());
    if (method == "GET" && p == "/v1/etn/reserve") {
      const auto r = store->latest_reserve();
      if (!r)
        return http_err(404, "no reserve");
      return http_ok(reserve_to_json(*r));
    }
    if (method == "POST" && p == "/v1/etn/reserve/refresh")
      return http_ok(refresh_reserve());
    if (method == "GET" && p == "/v1/etn/attestations") {
      const auto ids = store->list_attestation_ids();
      std::ostringstream o;
      o << "{\"ids\":[";
      for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i)
          o << ',';
        o << '"' << ids[i] << '"';
      }
      o << "]}";
      return http_ok(o.str());
    }
    if (method == "GET" && p.rfind("/v1/etn/attestations/", 0) == 0) {
      const auto id = p.substr(std::strlen("/v1/etn/attestations/"));
      const auto a = store->get_attestation(id);
      if (!a)
        return http_err(404, "not found");
      return http_ok(attestation_to_json(*a));
    }
    if (method == "POST" && p == "/v1/etn/attestations") {
      BurnMintAttestation a;
      if (!attestation_from_json(body, a))
        return http_err(400, "invalid attestation");
      if (!store->put_attestation(a))
        return http_err(500, "store failed");
      return http_ok("{\"id\":\"" + a.id + "\"}");
    }
    if (method == "POST" && p == "/json_rpc")
      return http_ok(handle_json_rpc(body));
    return http_err(404, "not found");
  }

  void serve(boost::asio::ip::tcp::socket socket)
  {
    try {
      boost::asio::streambuf buf;
      boost::asio::read_until(socket, buf, "\r\n\r\n");
      std::istream is{&buf};
      std::string request_line;
      std::getline(is, request_line);
      if (!request_line.empty() && request_line.back() == '\r')
        request_line.pop_back();
      std::string method, path, version;
      {
        std::istringstream rl{request_line};
        rl >> method >> path >> version;
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
        std::string value = header.substr(colon + 1);
        while (!value.empty() && value[0] == ' ')
          value.erase(value.begin());
        if (name == "Content-Length" || name == "content-length")
          content_length = static_cast<std::size_t>(std::stoul(value));
      }
      std::string body;
      if (content_length > 0) {
        body.resize(content_length);
        std::size_t have = buf.size();
        std::string already((std::istreambuf_iterator<char>(is)), {});
        if (already.size() >= content_length) {
          body = already.substr(0, content_length);
        } else {
          body = already;
          const auto need = content_length - body.size();
          std::vector<char> rest(need);
          boost::asio::read(socket, boost::asio::buffer(rest));
          body.append(rest.begin(), rest.end());
          (void)have;
        }
      }
      const auto resp = handle(method, path, body);
      boost::asio::write(socket, boost::asio::buffer(resp));
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

EtNServer::EtNServer() : impl_(std::make_unique<Impl>()) {}
EtNServer::~EtNServer() { stop(); }

void EtNServer::set_store(std::shared_ptr<EtNStore> store)
{
  if (store)
    impl_->store = std::move(store);
}

void EtNServer::set_token(std::string token) { impl_->token = std::move(token); }
void EtNServer::set_wallet_rpc_url(std::string url) { impl_->wallet_rpc_url = std::move(url); }

std::error_code EtNServer::listen(const std::string& host, const std::uint16_t port)
{
  stop();
  try {
    impl_->store->load();
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

void EtNServer::stop()
{
  if (!impl_)
    return;
  const auto port = impl_->port.load();
  const std::string host = impl_->host;
  impl_->running = false;
  if (port != 0) {
    try {
      boost::asio::io_context wake_io;
      boost::asio::ip::tcp::socket wake{wake_io};
      boost::system::error_code wake_ec;
      wake.connect({boost::asio::ip::make_address(host), port}, wake_ec);
      wake.close(wake_ec);
    } catch (...) {
    }
  }
  boost::system::error_code ec;
  impl_->acceptor.cancel(ec);
  impl_->acceptor.close(ec);
  impl_->io.stop();
  if (impl_->thread.joinable())
    impl_->thread.join();
  impl_->io.restart();
  impl_->port = 0;
}

std::uint16_t EtNServer::port() const noexcept { return impl_ ? impl_->port.load() : 0; }
bool EtNServer::running() const noexcept { return impl_ && impl_->running.load(); }
std::string EtNServer::base_url() const
{
  if (!running())
    return {};
  return "http://" + impl_->host + ":" + std::to_string(port());
}

bool parse_listen_address(const std::string& listen, std::string& host, std::uint16_t& port)
{
  const auto colon = listen.rfind(':');
  if (colon == std::string::npos || colon == 0)
    return false;
  host = listen.substr(0, colon);
  try {
    port = static_cast<std::uint16_t>(std::stoul(listen.substr(colon + 1)));
  } catch (...) {
    return false;
  }
  return !host.empty();
}

} // namespace arq_etn
