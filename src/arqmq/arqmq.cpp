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

#include "arqmq.h"

#include "mesh_bridge.hpp"
#include "socket_stack.hpp"
#include "transport.hpp"

#include <memory>
#include <mutex>
#include <system_error>

namespace {
struct facade_state
{
  std::mutex mutex;
  arqmq::Backend backend = arqmq::Backend::LegacyArqNet;
  arqmq::CategoryAcl default_acl = arqmq::CategoryAcl::Denied;
  bool initialized = false;
  std::unique_ptr<arqmq::SocketStack> socket_stack;
};

facade_state state;

int acl_rank(const arqmq::CategoryAcl acl) noexcept
{
  switch (acl) {
  case arqmq::CategoryAcl::Denied:
    return 0;
  case arqmq::CategoryAcl::Basic:
    return 1;
  case arqmq::CategoryAcl::ServiceNode:
    return 2;
  case arqmq::CategoryAcl::Admin:
    return 3;
  }
  return 0;
}

void stop_socket_stack_unlocked() noexcept
{
  if (state.socket_stack) {
    state.socket_stack->stop();
    state.socket_stack.reset();
  }
}
} // namespace

namespace arqmq {
const char* to_string(const Backend backend) noexcept
{
  switch (backend) {
  case Backend::LegacyArqNet:
    return "legacy-arqnet";
  case Backend::ArqMq:
    return "arqmq";
  }

  return "unknown";
}

const char* to_string(const CategoryAcl acl) noexcept
{
  switch (acl) {
  case CategoryAcl::Denied:
    return "denied";
  case CategoryAcl::Basic:
    return "basic";
  case CategoryAcl::ServiceNode:
    return "service-node";
  case CategoryAcl::Admin:
    return "admin";
  }

  return "unknown";
}

bool allows(const CategoryAcl required, const CategoryAcl granted) noexcept
{
  return acl_rank(granted) >= acl_rank(required);
}

std::error_code init(const Config& config) noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  stop_socket_stack_unlocked();
  state.backend = config.backend;
  state.default_acl = config.default_acl;
  state.initialized = false;

  switch (config.backend) {
  case Backend::LegacyArqNet:
    // Production mesh remains arqnet::SNNetwork (started from core::init).
    state.initialized = true;
    return {};
  case Backend::ArqMq: {
    try {
      auto stack = std::make_unique<SocketStack>();
      if (const auto ec = stack->start()) {
        state.backend = Backend::LegacyArqNet;
        return ec;
      }
      state.socket_stack = std::move(stack);
      attach_compatible_mesh_mirrors(*state.socket_stack);
      state.initialized = true;
      return {};
    } catch (...) {
      state.backend = Backend::LegacyArqNet;
      return std::make_error_code(std::errc::resource_unavailable_try_again);
    }
  }
  }

  return std::make_error_code(std::errc::invalid_argument);
}

std::error_code shutdown() noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  stop_socket_stack_unlocked();
  state.initialized = false;
  state.default_acl = CategoryAcl::Denied;
  return {};
}

Backend current_backend() noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  return state.backend;
}

const char* transport_name() noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  if (state.socket_stack && state.socket_stack->running())
    return k_transport_arqmq;
  return k_transport_snnetwork;
}

bool native_transport_active() noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  return state.socket_stack && state.socket_stack->running();
}

CategoryAcl default_acl() noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  return state.default_acl;
}

bool is_initialized() noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  return state.initialized;
}

/// Test/helper access to the active SocketStack (nullptr when legacy).
SocketStack* active_socket_stack() noexcept
{
  std::lock_guard<std::mutex> lock{state.mutex};
  return state.socket_stack.get();
}
} // namespace arqmq
