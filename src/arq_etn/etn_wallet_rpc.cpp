// Copyright (c) 2018 - 2026, The Arqma Network

#include "etn_wallet_rpc.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <sstream>

namespace arq_etn {
namespace {

bool parse_http_url(const std::string& wallet_rpc_url, std::string& host, std::uint16_t& port, std::string& path)
{
  if (wallet_rpc_url.rfind("http://", 0) != 0)
    return false;
  std::string url = wallet_rpc_url.substr(7);
  const auto slash = url.find('/');
  const std::string hostport = slash == std::string::npos ? url : url.substr(0, slash);
  path = slash == std::string::npos ? "/json_rpc" : url.substr(slash);
  if (path.empty() || path == "/")
    path = "/json_rpc";
  const auto colon = hostport.rfind(':');
  if (colon == std::string::npos)
    return false;
  host = hostport.substr(0, colon);
  try {
    port = static_cast<std::uint16_t>(std::stoul(hostport.substr(colon + 1)));
  } catch (...) {
    return false;
  }
  return !host.empty();
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

std::uint64_t extract_json_u64(const std::string& body, const std::string& key)
{
  const std::string needle = "\"" + key + "\":";
  const auto start = body.find(needle);
  if (start == std::string::npos)
    return 0;
  auto i = start + needle.size();
  while (i < body.size() && (body[i] == ' ' || body[i] == '\t'))
    ++i;
  std::uint64_t v = 0;
  while (i < body.size() && body[i] >= '0' && body[i] <= '9') {
    v = v * 10 + static_cast<std::uint64_t>(body[i] - '0');
    ++i;
  }
  return v;
}

std::string http_json_post(const std::string& host, std::uint16_t port, const std::string& path, const std::string& payload)
{
  boost::asio::io_context io;
  boost::asio::ip::tcp::socket sock{io};
  boost::asio::ip::tcp::resolver resolver{io};
  boost::asio::connect(sock, resolver.resolve(host, std::to_string(port)));
  const std::string hostport = host + ":" + std::to_string(port);
  const std::string req = "POST " + path + " HTTP/1.1\r\nHost: " + hostport +
                          "\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(payload.size()) +
                          "\r\nConnection: close\r\n\r\n" + payload;
  boost::asio::write(sock, boost::asio::buffer(req));
  boost::asio::streambuf buf;
  boost::system::error_code ec;
  boost::asio::read(sock, buf, boost::asio::transfer_all(), ec);
  std::istream is{&buf};
  std::string raw((std::istreambuf_iterator<char>(is)), {});
  const auto body_pos = raw.find("\r\n\r\n");
  if (body_pos == std::string::npos)
    return {};
  return raw.substr(body_pos + 4);
}

std::string json_rpc_call(const WalletRpcConfig& cfg, const std::string& method, const std::string& params_json)
{
  std::string host;
  std::uint16_t port = 0;
  std::string path;
  if (!parse_http_url(cfg.url, host, port, path))
    return {};
  const std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":\"0\",\"method\":\"" + method + "\",\"params\":" + params_json + "}";
  return http_json_post(host, port, path, payload);
}

} // namespace

ReserveProofResult fetch_reserve_proof_from_wallet_rpc(const WalletRpcConfig& cfg, const std::string& message)
{
  ReserveProofResult out;
  if (cfg.url.empty()) {
    out.error = "empty wallet-rpc url";
    return out;
  }
  try {
    const auto height_body = json_rpc_call(cfg, "get_height", "{}");
    out.wallet_height = extract_json_u64(height_body, "height");

    const auto bal_body = json_rpc_call(cfg, "get_balance", "{\"account_index\":0}");
    const auto bal = extract_json_u64(bal_body, "unlocked_balance");
    if (bal == 0) {
      const auto bal2 = extract_json_u64(bal_body, "balance");
      if (bal2)
        out.balance_atomic = std::to_string(bal2);
    } else {
      out.balance_atomic = std::to_string(bal);
    }

    std::ostringstream params;
    params << "{\"all\":true,\"message\":\"";
    for (unsigned char c : message) {
      if (c == '"' || c == '\\')
        params << '\\' << static_cast<char>(c);
      else
        params << static_cast<char>(c);
    }
    params << "\"}";
    const auto proof_body = json_rpc_call(cfg, "get_reserve_proof", params.str());
    if (proof_body.find("\"error\"") != std::string::npos && proof_body.find("\"result\"") == std::string::npos) {
      out.error = extract_json_string(proof_body, "message");
      if (out.error.empty())
        out.error = "wallet-rpc error";
      return out;
    }
    out.signature = extract_json_string(proof_body, "signature");
    out.ok = !out.signature.empty();
    if (!out.ok)
      out.error = "missing signature in get_reserve_proof response";
    return out;
  } catch (const std::exception& e) {
    out.error = e.what();
    return out;
  } catch (...) {
    out.error = "wallet-rpc call failed";
    return out;
  }
}

} // namespace arq_etn
