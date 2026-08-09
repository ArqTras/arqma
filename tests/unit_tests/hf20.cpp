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

#include "gtest/gtest.h"

#include "arqmq/arqmq.h"
#include "cryptonote_basic/hardfork.h"
#include "cryptonote_config.h"

TEST(hf20, feature_gate_aligns_with_arqmq_constant)
{
  ASSERT_EQ(HF_VERSION_NATIVE_ARQNET_MESH, cryptonote::network_version_20);
  ASSERT_EQ(arqmq::k_hf_native_arqnet_mesh, static_cast<uint8_t>(cryptonote::network_version_20));
}

TEST(hf20, schedules_stagenet_and_testnet_only)
{
  ASSERT_EQ(240u,
            cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::STAGENET, cryptonote::network_version_20));
  ASSERT_EQ(1300u,
            cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::TESTNET, cryptonote::network_version_20));
  // Mainnet height deliberately unscheduled until stagenet mesh parity.
  ASSERT_EQ(cryptonote::HardFork::INVALID_HF_VERSION_HEIGHT,
            cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::MAINNET, cryptonote::network_version_20));
}

TEST(hf20, mesh_cutover_requires_hf_and_implementation)
{
  EXPECT_FALSE(arqmq::hf_permits_native_mesh(static_cast<uint8_t>(cryptonote::network_version_19)));
  EXPECT_TRUE(arqmq::hf_permits_native_mesh(static_cast<uint8_t>(cryptonote::network_version_20)));
  EXPECT_FALSE(arqmq::native_mesh_implementation_ready());
  EXPECT_FALSE(arqmq::native_mesh_ready_at(static_cast<uint8_t>(cryptonote::network_version_20)));
  EXPECT_FALSE(arqmq::native_mesh_ready());
  EXPECT_STREQ("curve-zap-peer-relay-not-ported", arqmq::native_mesh_blocker());
}
