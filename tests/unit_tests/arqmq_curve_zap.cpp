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
#include "arqmq/curve_zap.hpp"
#include "arqmq/peer_table.hpp"
#include "arqmq/socket_stack.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <zmq.h>

namespace {
std::string make_pubkey(const char fill)
{
  return std::string(32, fill);
}

struct FrameScratch
{
  std::string version = "1.0";
  std::string req_id = "rid";
  std::string domain = arqmq::k_zap_auth_domain_sn;
  std::string address = "127.0.0.1";
  std::string identity;
  std::string mechanism = "CURVE";
  std::string creds;

  std::vector<std::string_view> views() const { return {version, req_id, domain, address, identity, mechanism, creds}; }
};
} // namespace

TEST(arqmq_curve_zap, accepts_service_node_and_denies_unknown)
{
  const auto pubkey = make_pubkey('A');
  const auto allow_sn = [](const std::string&, const std::string& pk) {
    return pk == make_pubkey('A') ? arqmq::CurvePeerAllow::ServiceNode : arqmq::CurvePeerAllow::Denied;
  };

  FrameScratch ok_frames;
  ok_frames.creds = pubkey;
  const auto ok = arqmq::evaluate_curve_zap_frames(ok_frames.views(), allow_sn);
  EXPECT_EQ("200", ok.status_code);
  EXPECT_EQ("S:" + arqmq::to_hex_lower(pubkey), ok.user_id);

  FrameScratch denied_frames;
  denied_frames.creds = make_pubkey('B');
  const auto denied = arqmq::evaluate_curve_zap_frames(denied_frames.views(), allow_sn);
  EXPECT_EQ("400", denied.status_code);
  EXPECT_TRUE(denied.user_id.empty());
}

TEST(arqmq_curve_zap, rejects_bad_domain_mechanism_and_key_size)
{
  const auto allow = [](const std::string&, const std::string&) { return arqmq::CurvePeerAllow::ServiceNode; };

  FrameScratch bad_domain;
  bad_domain.creds = make_pubkey('A');
  bad_domain.domain = "wrong.domain";
  EXPECT_EQ("400", arqmq::evaluate_curve_zap_frames(bad_domain.views(), allow).status_code);

  FrameScratch bad_mech;
  bad_mech.creds = make_pubkey('A');
  bad_mech.mechanism = "PLAIN";
  EXPECT_EQ("500", arqmq::evaluate_curve_zap_frames(bad_mech.views(), allow).status_code);

  FrameScratch short_key;
  short_key.creds.assign(31, 'x');
  EXPECT_EQ("500", arqmq::evaluate_curve_zap_frames(short_key.views(), allow).status_code);
}

TEST(arqmq_curve_zap, peer_table_tracks_sn_endpoints)
{
  arqmq::PeerTable table;
  const auto pk = make_pubkey('P');
  table.note_peer(pk, "tcp://127.0.0.1:19990", true);
  table.note_peer(std::string(31, 'x'), "tcp://bad", true); // ignored
  ASSERT_EQ(1u, table.size());
  const auto found = table.find(pk);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ("tcp://127.0.0.1:19990", found->hint);
  EXPECT_TRUE(found->service_node);
  EXPECT_TRUE(table.erase(pk));
  EXPECT_EQ(0u, table.size());
}

TEST(arqmq_curve_zap, socket_stack_bind_curve_and_send_stub)
{
  char public_key[41]{};
  char secret_key[41]{};
  ASSERT_EQ(0, zmq_curve_keypair(public_key, secret_key));
  // libzmq returns Z85; convert to binary 32-byte keys for sockopt.
  std::array<uint8_t, 32> pub_bin{};
  std::array<uint8_t, 32> sec_bin{};
  ASSERT_NE(nullptr, zmq_z85_decode(pub_bin.data(), public_key));
  ASSERT_NE(nullptr, zmq_z85_decode(sec_bin.data(), secret_key));

  arqmq::SocketStack stack;
  ASSERT_FALSE(stack.start());
  stack.set_curve_identity(std::string(reinterpret_cast<const char*>(pub_bin.data()), pub_bin.size()),
                           std::string(reinterpret_cast<const char*>(sec_bin.data()), sec_bin.size()));
  stack.set_allow_connection([](const std::string&, const std::string&) { return arqmq::CurvePeerAllow::ServiceNode; });
  ASSERT_TRUE(stack.curve_zap_configured());

  const std::string endpoint = "inproc://arqmq.curve.bind." + std::to_string(reinterpret_cast<uintptr_t>(&stack));
  EXPECT_FALSE(stack.bind_curve(endpoint));
  stack.peers().note_peer(make_pubkey('Z'), "tcp://127.0.0.1:1", true);
  EXPECT_EQ(1u, stack.peers().size());
  EXPECT_EQ(std::errc::operation_not_supported, stack.send_to_peer(make_pubkey('Z'), "vote_ob", "x"));
  stack.stop();
}

TEST(arqmq_curve_zap, native_mesh_blocker_advances_past_curve_zap)
{
  EXPECT_FALSE(arqmq::native_mesh_implementation_ready());
  EXPECT_STREQ("peer-send-path-missing", arqmq::native_mesh_blocker());
  EXPECT_TRUE(arqmq::peer_mesh_is_snnetwork());
}
