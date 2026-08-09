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

namespace arqmq {
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
// 1 = Curve/ZAP on SocketStack landed; peer table missing
// 2 = peer table landed; outbound send / vote_ob relay missing
// 3 = vote_ob relay + stagenet parity verified → implementation ready
constexpr int k_native_mesh_port_stage = 2;
} // namespace

bool native_mesh_implementation_ready() noexcept
{
  return k_native_mesh_port_stage >= 3;
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
} // namespace arqmq
