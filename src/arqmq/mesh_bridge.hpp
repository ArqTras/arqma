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

#include "curve_zap.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace arqmq {
class SocketStack;

/// Attaches dual-run command mirrors on the dedicated SocketStack without
/// replacing SNNetwork wire handlers. No-op when native transport is inactive.
void attach_compatible_mesh_mirrors(SocketStack& stack);

/// Convenience: attach mirrors when `active_socket_stack()` is running.
void attach_compatible_mesh_mirrors_if_active();

/// Loads CURVE identity + ZAP allow onto the active SocketStack for shadow mesh
/// work. Does not bind a listener (SNNetwork keeps the live arqnet port).
void configure_mesh_shadow(SocketStack& stack, std::string public_key, std::string secret_key, AllowConnection allow);

/// Opt-in dual-write of peer commands onto SocketStack (default off).
bool native_mesh_shadow_relay_enabled() noexcept;
void set_native_mesh_shadow_relay_enabled(bool enabled) noexcept;

struct MeshShadowStats
{
  uint64_t attempts = 0;
  uint64_t ok = 0;
  uint64_t fail = 0;
};

/// Cumulative shadow-send counters (reset when shadow is toggled on).
MeshShadowStats native_mesh_shadow_stats() noexcept;

/// Best-effort shadow send; never throws. No-op unless shadow relay is enabled
/// and the active stack has CURVE identity configured.
void shadow_send_to_peer(std::string_view pubkey, std::string_view command, std::string_view payload,
                         std::string_view hint = {});
} // namespace arqmq
