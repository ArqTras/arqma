// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "storage_client.h"
#include "http_io.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>
#include <array>
#include <chrono>
#include <mutex>
#include <set>
#include <sstream>
#include <utility>

namespace {
std::error_code not_connected() noexcept
{
  return std::make_error_code(std::errc::not_connected);
}

std::error_code invalid_argument() noexcept
{
  return std::make_error_code(std::errc::invalid_argument);
}

bool tcp_connect_only(const arq_storage::Endpoint& endpoint) noexcept
{
  try {
    boost::asio::io_context io;
    boost::asio::ip::tcp::resolver resolver{io};
    const auto results = resolver.resolve(endpoint.host, std::to_string(endpoint.port));
    boost::asio::ip::tcp::socket socket{io};
    boost::system::error_code ec;
    boost::asio::connect(socket, results, ec);
    if (ec)
      return false;
    socket.close();
    return true;
  } catch (...) {
    return false;
  }
}

/// Cleartext HTTP GET probe. Any response starting with "HTTP/" counts as
/// reachable (including 4xx). TLS endpoints stay on TCP-only until a TLS
/// client stack is wired into arq_storage.
bool http_get_probe(const arq_storage::Endpoint& endpoint) noexcept
{
  try {
    const auto request = arq_storage::format_http_get_request(endpoint);
    if (request.empty())
      return false;

    boost::asio::io_context io;
    boost::asio::ip::tcp::resolver resolver{io};
    const auto results = resolver.resolve(endpoint.host, std::to_string(endpoint.port));
    boost::asio::ip::tcp::socket socket{io};
    boost::system::error_code ec;
    boost::asio::connect(socket, results, ec);
    if (ec)
      return false;

    boost::asio::write(socket, boost::asio::buffer(request), ec);
    if (ec)
      return false;

    std::array<char, 16> buf{};
    const std::size_t n = boost::asio::read(socket, boost::asio::buffer(buf), boost::asio::transfer_at_least(5), ec);
    socket.close();
    if (n < 5)
      return false;
    return std::string_view{buf.data(), 5} == "HTTP/";
  } catch (...) {
    return false;
  }
}

std::mutex g_daemon_mutex;
arq_storage::Config g_daemon_config{};
} // namespace

namespace arq_storage {
StorageClient::StorageClient(Config config) noexcept
    : config_{std::move(config)}, endpoint_{parse_endpoint(config_.base_url)}
{}

std::error_code StorageClient::ping() const noexcept
{
  if (config_.backend == Backend::InMemory)
    return {};

  if (!endpoint_)
    return not_connected();

  if (endpoint_.tls)
    return tcp_connect_only(endpoint_) ? std::error_code{} : not_connected();

  return http_get_probe(endpoint_) ? std::error_code{} : not_connected();
}

std::error_code StorageClient::store(const StoreRequest& request) noexcept
{
  if (request.namespace_name.empty() || request.key.empty())
    return invalid_argument();
  if (config_.backend == Backend::InMemory) {
    std::lock_guard<std::mutex> lock{mutex_};
    values_[{request.namespace_name, request.key}] = request.value;
    return {};
  }
  if (!endpoint_ || endpoint_.tls)
    return not_connected();
  auto path = kv_path(request.namespace_name, request.key);
  if (request.ttl_seconds != 0)
    path += "&ttl=" + std::to_string(request.ttl_seconds);
  const auto result = http_exchange(endpoint_, "PUT", path, request.value, config_.connect_timeout);
  if (!result)
    return result.error ? result.error : not_connected();
  return {};
}

Result<std::string> StorageClient::retrieve(std::string namespace_name, std::string key) const noexcept
{
  if (namespace_name.empty() || key.empty())
    return {{}, invalid_argument()};
  if (config_.backend == Backend::InMemory) {
    std::lock_guard<std::mutex> lock{mutex_};
    const auto it = values_.find({namespace_name, key});
    if (it == values_.end())
      return {{}, std::make_error_code(std::errc::no_such_file_or_directory)};
    return {it->second, {}};
  }
  if (!endpoint_ || endpoint_.tls)
    return {{}, not_connected()};
  const auto result = http_exchange(endpoint_, "GET", kv_path(namespace_name, key), {}, config_.connect_timeout);
  if (result.status != 404 && result)
    return {result.body, {}};
  if (result.status != 404 && result.error)
    return {{}, result.error};
  for (const auto& ep : inbox_swarm_endpoints(namespace_name)) {
    const auto replica = http_exchange(ep, "GET", kv_path(namespace_name, key), {}, config_.connect_timeout);
    if (replica)
      return {replica.body, {}};
  }
  if (result.status == 404)
    return {{}, std::make_error_code(std::errc::no_such_file_or_directory)};
  return {{}, result.error ? result.error : not_connected()};
}

Result<std::vector<std::string>> StorageClient::list_keys(std::string namespace_name) const noexcept
{
  if (namespace_name.empty())
    return {{}, invalid_argument()};
  if (config_.backend == Backend::InMemory) {
    std::vector<std::string> keys;
    std::lock_guard<std::mutex> lock{mutex_};
    for (const auto& kv : values_) {
      if (kv.first.first == namespace_name)
        keys.push_back(kv.first.second);
    }
    return {keys, {}};
  }
  if (!endpoint_ || endpoint_.tls)
    return {{}, not_connected()};
  const auto result =
      http_exchange(endpoint_, "GET", "/v1/list?ns=" + url_encode(namespace_name), {}, config_.connect_timeout);
  if (!result)
    return {{}, result.error ? result.error : not_connected()};
  std::set<std::string> unique;
  std::string line;
  std::istringstream iss{result.body};
  while (std::getline(iss, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (!line.empty())
      unique.insert(std::move(line));
  }
  for (const auto& ep : inbox_swarm_endpoints(namespace_name)) {
    const auto replica =
        http_exchange(ep, "GET", "/v1/list?ns=" + url_encode(namespace_name), {}, config_.connect_timeout);
    if (!replica)
      continue;
    std::istringstream replica_iss{replica.body};
    while (std::getline(replica_iss, line)) {
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      if (!line.empty())
        unique.insert(std::move(line));
    }
  }
  return {{unique.begin(), unique.end()}, {}};
}

Result<std::vector<std::string>> StorageClient::get_snodes_for_pubkey(std::string pubkey) const noexcept
{
  if (pubkey.empty())
    return {{}, invalid_argument()};
  if (config_.backend == Backend::InMemory) {
    std::lock_guard<std::mutex> lock{mutex_};
    const auto it = snodes_.find(pubkey);
    if (it == snodes_.end())
      return {{}, {}};
    return {it->second, {}};
  }
  if (!endpoint_ || endpoint_.tls)
    return {{}, not_connected()};
  const auto result = http_exchange(endpoint_, "GET", snodes_path(pubkey), {}, config_.connect_timeout);
  if (!result)
    return {{}, result.error ? result.error : not_connected()};
  std::vector<std::string> snodes;
  std::string line;
  std::istringstream iss{result.body};
  while (std::getline(iss, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (!line.empty())
      snodes.push_back(std::move(line));
  }
  return {snodes, {}};
}

void StorageClient::set_snodes_for_pubkey(std::string pubkey, std::vector<std::string> snodes)
{
  if (config_.backend == Backend::InMemory) {
    std::lock_guard<std::mutex> lock{mutex_};
    snodes_[std::move(pubkey)] = std::move(snodes);
    return;
  }
  if (!endpoint_ || endpoint_.tls)
    return;
  std::string body;
  for (const auto& sn : snodes) {
    body.append(sn);
    body.push_back('\n');
  }
  http_exchange(endpoint_, "PUT", snodes_path(pubkey), body, config_.connect_timeout);
}

std::vector<Endpoint> StorageClient::inbox_swarm_endpoints(const std::string& namespace_name) const
{
  std::vector<Endpoint> out;
  if (config_.backend != Backend::Remote || !endpoint_ || endpoint_.tls)
    return out;
  const auto pub = inbox_pubkey(namespace_name);
  if (pub.empty())
    return out;
  const auto members = get_snodes_for_pubkey(pub);
  if (!members)
    return out;
  const auto self = format_http_authority(endpoint_.host, endpoint_.port);
  for (const auto& url : members.value) {
    const auto ep = parse_endpoint(url);
    if (!ep || ep.tls)
      continue;
    if (format_http_authority(ep.host, ep.port) == self)
      continue;
    out.push_back(ep);
    if (out.size() >= max_swarm_fallback)
      break;
  }
  return out;
}

void configure_daemon_client(Config config)
{
  std::lock_guard<std::mutex> lock{g_daemon_mutex};
  g_daemon_config = std::move(config);
}

StorageClient daemon_client()
{
  std::lock_guard<std::mutex> lock{g_daemon_mutex};
  return StorageClient{g_daemon_config};
}
} // namespace arq_storage
