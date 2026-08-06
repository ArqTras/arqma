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
#include "arqmq/backend_policy.hpp"
#include "arqmq/socket_stack.hpp"
#include "arqmq/transport.hpp"

TEST(arqmq_compat, default_config_is_legacy_snnetwork)
{
  const arqmq::Config defaults{};
  EXPECT_EQ(arqmq::Backend::LegacyArqNet, defaults.backend);
  EXPECT_EQ(arqmq::CategoryAcl::Denied, defaults.default_acl);
}

TEST(arqmq_compat, mainnet_refuses_arqmq_without_override)
{
  const auto sel = arqmq::resolve_backend("arqmq", arqmq::NetworkClass::Mainnet, false);
  EXPECT_EQ(arqmq::Backend::LegacyArqNet, sel.backend);
  EXPECT_TRUE(sel.overridden);
  EXPECT_TRUE(arqmq::peer_mesh_is_snnetwork());
}

TEST(arqmq_compat, mainnet_allows_arqmq_only_with_explicit_override)
{
  const auto sel = arqmq::resolve_backend("arqmq", arqmq::NetworkClass::Mainnet, true);
  EXPECT_EQ(arqmq::Backend::ArqMq, sel.backend);
  EXPECT_FALSE(sel.overridden);
}

TEST(arqmq_compat, testnet_allows_experimental_arqmq)
{
  const auto sel = arqmq::resolve_backend("arqmq", arqmq::NetworkClass::Testnet, false);
  EXPECT_EQ(arqmq::Backend::ArqMq, sel.backend);
  EXPECT_FALSE(sel.overridden);
}

TEST(arqmq_compat, mainnet_default_and_unknown_stay_legacy)
{
  EXPECT_EQ(arqmq::Backend::LegacyArqNet,
            arqmq::resolve_backend("legacy-arqnet", arqmq::NetworkClass::Mainnet).backend);
  EXPECT_EQ(arqmq::Backend::LegacyArqNet,
            arqmq::resolve_backend("", arqmq::NetworkClass::Mainnet).backend);
  const auto unknown = arqmq::resolve_backend("nope", arqmq::NetworkClass::Mainnet);
  EXPECT_EQ(arqmq::Backend::LegacyArqNet, unknown.backend);
  EXPECT_TRUE(unknown.overridden);
}

TEST(arqmq_compat, legacy_path_never_starts_native_stack)
{
  EXPECT_FALSE(arqmq::shutdown());
  EXPECT_FALSE(arqmq::init(arqmq::Config{arqmq::Backend::LegacyArqNet, arqmq::CategoryAcl::Basic}));
  EXPECT_EQ(arqmq::Backend::LegacyArqNet, arqmq::current_backend());
  EXPECT_FALSE(arqmq::native_transport_active());
  EXPECT_EQ(nullptr, arqmq::active_socket_stack());
  EXPECT_STREQ(arqmq::k_transport_snnetwork, arqmq::transport_name());
  EXPECT_STREQ(arqmq::k_transport_snnetwork, arqmq::mesh_transport_name());
  EXPECT_TRUE(arqmq::peer_mesh_is_snnetwork());
  EXPECT_FALSE(arqmq::shutdown());
}

TEST(arqmq_compat, arqmq_keeps_peer_mesh_on_snnetwork)
{
  EXPECT_FALSE(arqmq::shutdown());
  EXPECT_FALSE(arqmq::init(arqmq::Config{arqmq::Backend::ArqMq, arqmq::CategoryAcl::ServiceNode}));
  EXPECT_EQ(arqmq::Backend::ArqMq, arqmq::current_backend());
  EXPECT_TRUE(arqmq::native_transport_active());
  EXPECT_STREQ(arqmq::k_transport_arqmq, arqmq::transport_name());
  EXPECT_STREQ(arqmq::k_transport_snnetwork, arqmq::mesh_transport_name());
  EXPECT_TRUE(arqmq::peer_mesh_is_snnetwork());

  auto* stack = arqmq::active_socket_stack();
  ASSERT_NE(nullptr, stack);
  EXPECT_TRUE(stack->has_handler("vote_ob"));
  EXPECT_TRUE(stack->has_handler("arqnet_status"));

  arqmq::InboundRequest vote;
  vote.command = "vote_ob";
  vote.peer_acl = arqmq::CategoryAcl::ServiceNode;
  vote.payload = "compat";
  std::string reply;
  EXPECT_FALSE(stack->dispatch(vote, &reply));
  EXPECT_EQ(arqmq::k_transport_snnetwork, reply);

  arqmq::InboundRequest status;
  status.command = "arqnet_status";
  status.peer_acl = arqmq::CategoryAcl::Basic;
  EXPECT_FALSE(stack->dispatch(status, &reply));
  EXPECT_NE(std::string::npos, reply.find("mesh=snnetwork"));
  EXPECT_NE(std::string::npos, reply.find("transport=arqmq"));

  EXPECT_FALSE(arqmq::shutdown());
}

TEST(arqmq_compat, backend_switch_restores_legacy_without_native_stack)
{
  EXPECT_FALSE(arqmq::shutdown());
  EXPECT_FALSE(arqmq::init(arqmq::Config{arqmq::Backend::ArqMq, arqmq::CategoryAcl::Basic}));
  EXPECT_TRUE(arqmq::native_transport_active());

  EXPECT_FALSE(arqmq::init(arqmq::Config{arqmq::Backend::LegacyArqNet, arqmq::CategoryAcl::Basic}));
  EXPECT_EQ(arqmq::Backend::LegacyArqNet, arqmq::current_backend());
  EXPECT_FALSE(arqmq::native_transport_active());
  EXPECT_STREQ(arqmq::k_transport_snnetwork, arqmq::transport_name());
  EXPECT_TRUE(arqmq::peer_mesh_is_snnetwork());
  EXPECT_FALSE(arqmq::shutdown());
}
