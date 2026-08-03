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

#include "arq_messaging/identity.hpp"
#include "arq_messaging/onion_request.hpp"
#include "arq_messaging/swarm_map.hpp"
#include "arq_router/router_service.h"
#include "rpc/rpc_auth.h"

TEST(arq_messaging_swarm, in_memory_mapping_roundtrip)
{
  arq_messaging::InMemorySwarmMap map;
  arq_messaging::SwarmMapping mapping;
  mapping.id = 42;
  mapping.service_nodes = {"a", "b"};
  map.set_mapping("pk", mapping);

  EXPECT_FALSE(map.refresh());
  const auto got = map.get_swarm("pk");
  ASSERT_TRUE(got);
  EXPECT_EQ(42u, got.id);
  ASSERT_EQ(2u, got.service_nodes.size());
}

TEST(arq_messaging_onion, validates_complete_three_hop_request)
{
  arq_messaging::OnionRequest req;
  req.endpoint = "/store";
  for (auto &hop : req.hops)
  {
    hop.service_node_pubkey = "pk";
    hop.address = "127.0.0.1";
    hop.port = 19994;
  }
  EXPECT_TRUE(arq_messaging::validate_onion_request(req));
  req.hops[1].port = 0;
  EXPECT_FALSE(arq_messaging::validate_onion_request(req));
}

TEST(arq_messaging_identity, generate_curve25519_keypair)
{
  arq_messaging::Identity a{};
  arq_messaging::Identity b{};
  EXPECT_FALSE(arq_messaging::generate_identity(a));
  EXPECT_FALSE(arq_messaging::generate_identity(b));
  EXPECT_FALSE(a.public_key.is_null());
  EXPECT_NE(a.public_key.data, b.public_key.data);
}

TEST(arq_router, experimental_lifecycle_when_enabled)
{
  arq_router::RouterConfig cfg;
  cfg.enabled = true;
  arq_router::RouterService router{cfg};
  EXPECT_FALSE(router.init());
  EXPECT_FALSE(router.start());
  EXPECT_TRUE(router.running());
  EXPECT_FALSE(router.stop());
  EXPECT_FALSE(router.running());
}

TEST(arq_router, disabled_rejects_init)
{
  arq_router::RouterService router{};
  EXPECT_EQ(std::make_error_code(std::errc::operation_not_permitted), router.init());
}

TEST(rpc_auth, access_levels_and_operator_methods)
{
  using cryptonote::rpc::AccessLevel;
  EXPECT_TRUE(cryptonote::rpc::access_allows(AccessLevel::Public, AccessLevel::Admin));
  EXPECT_FALSE(cryptonote::rpc::access_allows(AccessLevel::Admin, AccessLevel::Public));
  EXPECT_TRUE(cryptonote::rpc::method_requires_operator("start_mining"));
  EXPECT_FALSE(cryptonote::rpc::method_requires_operator("get_info"));
}
