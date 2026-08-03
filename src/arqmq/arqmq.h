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

#include <system_error>

namespace arqmq {
/// Messaging facade backends. Both currently delegate mesh I/O to the
/// production `arqnet::SNNetwork` path; `ArqMq` selects the Arqma-named
/// command/ACL facade while transport remains SNNetwork until a dedicated
/// socket stack is ported.
enum class Backend
{
  LegacyArqNet,
  ArqMq
};

enum class CategoryAcl
{
  Denied,
  Basic,
  ServiceNode,
  Admin
};

struct Config
{
  Backend backend = Backend::LegacyArqNet;
  CategoryAcl default_acl = CategoryAcl::Denied;
};

const char* to_string(Backend backend) noexcept;
const char* to_string(CategoryAcl acl) noexcept;

/// Returns true when `granted` is at least as privileged as `required`.
/// Ordering: Denied < Basic < ServiceNode < Admin.
bool allows(CategoryAcl required, CategoryAcl granted) noexcept;

/// Initializes the messaging facade. Both backends mark the facade ready and
/// document that live Curve/ZMQ transport continues via SNNetwork.
std::error_code init(const Config& config = {}) noexcept;

/// Shuts down the messaging facade.
std::error_code shutdown() noexcept;

/// Returns the currently selected backend, even if initialization failed.
Backend current_backend() noexcept;

/// Stable name of the active wire transport under the facade.
const char* transport_name() noexcept;

/// Returns the default ACL configured at init (Denied after shutdown).
CategoryAcl default_acl() noexcept;

/// Returns true when the facade is initialized and ready for use.
bool is_initialized() noexcept;
} // namespace arqmq
