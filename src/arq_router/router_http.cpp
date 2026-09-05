// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "router_http.h"
#include "arq_messaging/onion_layer.hpp"

#include <sstream>
#include <vector>

namespace arq_router {
namespace {
std::string hex32(const arq_messaging::X25519PublicKey& pub)
{
  static const char* digits = "0123456789abcdef";
  std::string out(64, '0');
  for (std::size_t i = 0; i < 32; ++i) {
    out[i * 2] = digits[pub.data[i] >> 4];
    out[i * 2 + 1] = digits[pub.data[i] & 0xf];
  }
  return out;
}

std::string http_ok(const std::string& body, const char* type = "application/octet-stream")
{
  std::ostringstream oss;
  oss << "HTTP/1.1 200 OK\r\nContent-Type: " << type << "\r\nContent-Length: " << body.size()
      << "\r\nConnection: close\r\n\r\n"
      << body;
  return oss.str();
}
} // namespace

std::string handle_http(const std::string& method, const std::string& path, const std::string& body,
                        const arq_messaging::Identity& hop)
{
  const auto path_only = path.substr(0, path.find('?'));
  if (method == "GET" && (path_only == "/" || path_only == "/status"))
    return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\nConnection: close\r\n\r\narqma-router";

  if (method == "GET" && path_only == "/v1/pubkey")
    return http_ok(hex32(hop.public_key), "text/plain");

  if (method == "POST" && path_only == "/v1/peel") {
    if (body.empty() || hop.public_key.is_null())
      return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    std::vector<std::uint8_t> outer(body.begin(), body.end());
    std::vector<std::uint8_t> inner;
    if (arq_messaging::peel_onion_layer(hop, outer, inner))
      return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    return http_ok(std::string(inner.begin(), inner.end()));
  }

  return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
}
} // namespace arq_router
