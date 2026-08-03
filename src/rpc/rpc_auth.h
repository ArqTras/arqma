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

#pragma once

#include <string_view>

namespace cryptonote
{
namespace rpc
{
  /// Coarse RPC privilege levels. Restricted public daemons map remote callers
  /// to Public; unrestricted daemons grant Operator to RPC origins; in-process
  /// calls (no connection context) are treated as Admin.
  enum class AccessLevel
  {
    Public = 0,
    Operator = 1,
    Admin = 2
  };

  inline bool access_allows(AccessLevel required, AccessLevel granted) noexcept
  {
    return static_cast<int>(granted) >= static_cast<int>(required);
  }

  inline AccessLevel daemon_access_level(bool restricted_mode, bool has_rpc_origin) noexcept
  {
    if (!has_rpc_origin)
      return AccessLevel::Admin;
    return restricted_mode ? AccessLevel::Public : AccessLevel::Operator;
  }

  inline bool method_requires_operator(std::string_view method) noexcept
  {
    return method == "start_mining"
        || method == "stop_mining"
        || method == "stop_daemon"
        || method == "set_bans"
        || method == "flush_txpool"
        || method == "relay_tx"
        || method == "set_log_level"
        || method == "set_log_categories"
        || method == "save_bc"
        || method == "pop_blocks"
        || method == "prune_blockchain";
  }

  inline bool allow_rpc_method(std::string_view method, AccessLevel granted) noexcept
  {
    if (!method_requires_operator(method))
      return true;
    return access_allows(AccessLevel::Operator, granted);
  }
}
}
