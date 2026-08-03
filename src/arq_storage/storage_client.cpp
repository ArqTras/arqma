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

#include <utility>

namespace
{
  std::error_code not_connected() noexcept
  {
    return std::make_error_code(std::errc::not_connected);
  }

  std::error_code invalid_argument() noexcept
  {
    return std::make_error_code(std::errc::invalid_argument);
  }
}

namespace arq_storage
{
  StorageClient::StorageClient(const Backend backend) noexcept
    : backend_{backend}
  {}

  std::error_code StorageClient::ping() const noexcept
  {
    if (backend_ == Backend::InMemory)
      return {};
    return not_connected();
  }

  std::error_code StorageClient::store(const StoreRequest &request) noexcept
  {
    if (backend_ != Backend::InMemory)
      return not_connected();
    if (request.namespace_name.empty() || request.key.empty())
      return invalid_argument();

    std::lock_guard<std::mutex> lock{mutex_};
    values_[{request.namespace_name, request.key}] = request.value;
    return {};
  }

  Result<std::string> StorageClient::retrieve(std::string namespace_name, std::string key) const noexcept
  {
    if (backend_ != Backend::InMemory)
      return {{}, not_connected()};
    if (namespace_name.empty() || key.empty())
      return {{}, invalid_argument()};

    std::lock_guard<std::mutex> lock{mutex_};
    const auto it = values_.find({namespace_name, key});
    if (it == values_.end())
      return {{}, std::make_error_code(std::errc::no_such_file_or_directory)};
    return {it->second, {}};
  }

  Result<std::vector<std::string>> StorageClient::get_snodes_for_pubkey(std::string pubkey) const noexcept
  {
    if (backend_ != Backend::InMemory)
      return {{}, not_connected()};
    if (pubkey.empty())
      return {{}, invalid_argument()};

    std::lock_guard<std::mutex> lock{mutex_};
    const auto it = snodes_.find(pubkey);
    if (it == snodes_.end())
      return {{}, {}};
    return {it->second, {}};
  }

  void StorageClient::set_snodes_for_pubkey(std::string pubkey, std::vector<std::string> snodes)
  {
    std::lock_guard<std::mutex> lock{mutex_};
    snodes_[std::move(pubkey)] = std::move(snodes);
  }
}
