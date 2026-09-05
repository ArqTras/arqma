// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#pragma once

#include "storage_endpoint.h"

#include <chrono>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace arq_storage {
struct HttpResult
{
  int status = 0;
  std::string body;
  std::error_code error{};
  explicit operator bool() const noexcept { return !error && status >= 200 && status < 300; }
};

std::string url_encode(std::string_view raw);
std::string url_decode(std::string_view raw);
std::string query_get(std::string_view path, std::string_view key);
std::string kv_path(std::string_view ns, std::string_view key);
std::string snodes_path(std::string_view pubkey);

std::string format_http_request(std::string_view method, std::string_view path, std::string_view host,
                                std::string_view body);

HttpResult http_exchange(const Endpoint& endpoint, std::string_view method, std::string_view path,
                         std::string_view body, std::chrono::milliseconds timeout = std::chrono::milliseconds{2000});
} // namespace arq_storage
