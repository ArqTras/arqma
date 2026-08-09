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

#include "mesh_bridge.hpp"

#include "arqmq.h"
#include "socket_stack.hpp"
#include "transport.hpp"

#include <atomic>
#include <cstdint>

namespace arqmq {
namespace {
std::atomic<bool> g_shadow_relay_enabled{false};
std::atomic<uint64_t> g_shadow_attempts{0};
std::atomic<uint64_t> g_shadow_ok{0};
std::atomic<uint64_t> g_shadow_fail{0};

void reset_shadow_stats() noexcept
{
  g_shadow_attempts.store(0, std::memory_order_relaxed);
  g_shadow_ok.store(0, std::memory_order_relaxed);
  g_shadow_fail.store(0, std::memory_order_relaxed);
}
} // namespace
const char* mesh_transport_name() noexcept
{
  // Compatibility lock: peer wire stays on SNNetwork for both backends until
  // dual-run cutover flips this deliberately.
  return k_transport_snnetwork;
}

bool peer_mesh_is_snnetwork() noexcept
{
  // Live peer carrier stays SNNetwork until native_mesh_ready_at(hf) is true.
  return true;
}

bool hf_permits_native_mesh(const uint8_t hard_fork_version) noexcept
{
  return hard_fork_version >= k_hf_native_arqnet_mesh;
}

namespace {
// Incremental native-mesh port stages (compile-time progress; cutover stays off).
// 0 = missing Curve/ZAP allow path
// 1 = Curve/ZAP landed; peer table missing
// 2 = peer table landed; outbound send missing
// 3 = peer send landed; stagenet vote_ob parity / daemon wiring unverified
// 4 = implementation ready (cutover still needs HF20+)
constexpr int k_native_mesh_port_stage = 3;
} // namespace

bool native_mesh_implementation_ready() noexcept
{
  return k_native_mesh_port_stage >= 4;
}

bool native_mesh_ready_at(const uint8_t hard_fork_version) noexcept
{
  return hf_permits_native_mesh(hard_fork_version) && native_mesh_implementation_ready();
}

bool native_mesh_ready() noexcept
{
  // Without a chain height, only the implementation gate applies.
  return native_mesh_implementation_ready();
}

const char* native_mesh_blocker() noexcept
{
  if (k_native_mesh_port_stage < 1)
    return "curve-zap-peer-relay-not-ported";
  if (k_native_mesh_port_stage < 2)
    return "peer-endpoints-missing";
  if (k_native_mesh_port_stage < 3)
    return "peer-send-path-missing";
  if (k_native_mesh_port_stage < 4)
    return "vote-ob-parity-unverified";
  return "hf-below-native-mesh";
}

void attach_compatible_mesh_mirrors(SocketStack& stack)
{
  // Mirror the production command set for dual-run ACL/framing checks.
  // Replies make ownership of the live peer mesh explicit: SNNetwork.
  stack.register_handler("vote_ob", CategoryAcl::ServiceNode,
                         [](const InboundRequest&) { return std::string{k_transport_snnetwork}; });
  stack.register_handler("ping", CategoryAcl::Basic, [](const InboundRequest&) { return std::string{"pong"}; });
  stack.register_handler("pong", CategoryAcl::Basic, [](const InboundRequest&) { return std::string{}; });
  stack.register_handler("arqnet_status", CategoryAcl::Basic, [](const InboundRequest&) {
    return std::string{"backend="} + to_string(current_backend()) + ";transport=" + transport_name() +
           ";mesh=" + mesh_transport_name();
  });
}

void attach_compatible_mesh_mirrors_if_active()
{
  if (auto* stack = active_socket_stack())
    attach_compatible_mesh_mirrors(*stack);
}

void configure_mesh_shadow(SocketStack& stack, std::string public_key, std::string secret_key, AllowConnection allow)
{
  stack.set_curve_identity(std::move(public_key), std::move(secret_key));
  stack.set_allow_connection(std::move(allow));
}

bool native_mesh_shadow_relay_enabled() noexcept
{
  return g_shadow_relay_enabled.load(std::memory_order_relaxed);
}

void set_native_mesh_shadow_relay_enabled(const bool enabled) noexcept
{
  if (enabled)
    reset_shadow_stats();
  g_shadow_relay_enabled.store(enabled, std::memory_order_relaxed);
}

MeshShadowStats native_mesh_shadow_stats() noexcept
{
  return MeshShadowStats{g_shadow_attempts.load(std::memory_order_relaxed), g_shadow_ok.load(std::memory_order_relaxed),
                         g_shadow_fail.load(std::memory_order_relaxed)};
}

void shadow_send_to_peer(const std::string_view pubkey, const std::string_view command, const std::string_view payload,
                         const std::string_view hint)
{
  if (!native_mesh_shadow_relay_enabled())
    return;
  auto* stack = active_socket_stack();
  if (!stack || !stack->curve_zap_configured() || pubkey.size() != 32 || command.empty()) {
    g_shadow_attempts.fetch_add(1, std::memory_order_relaxed);
    g_shadow_fail.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  g_shadow_attempts.fetch_add(1, std::memory_order_relaxed);
  if (!hint.empty())
    stack->peers().note_peer(std::string{pubkey}, std::string{hint}, true);
  if (stack->send_to_peer(pubkey, command, payload, hint))
    g_shadow_fail.fetch_add(1, std::memory_order_relaxed);
  else
    g_shadow_ok.fetch_add(1, std::memory_order_relaxed);
}
} // namespace arqmq
