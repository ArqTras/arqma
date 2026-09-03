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
#include "arqmq/mesh_bridge.hpp"
#include "arqmq/peer_table.hpp"
#include "arqmq/socket_stack.hpp"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
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

bool make_curve_keypair(std::string& pub_bin, std::string& sec_bin)
{
  char public_key[41]{};
  char secret_key[41]{};
  if (zmq_curve_keypair(public_key, secret_key) != 0)
    return false;
  std::array<uint8_t, 32> pub{};
  std::array<uint8_t, 32> sec{};
  if (!zmq_z85_decode(pub.data(), public_key) || !zmq_z85_decode(sec.data(), secret_key))
    return false;
  pub_bin.assign(reinterpret_cast<const char*>(pub.data()), pub.size());
  sec_bin.assign(reinterpret_cast<const char*>(sec.data()), sec.size());
  return true;
}

TEST(arqmq_curve_zap, socket_stack_bind_curve_and_peer_table)
{
  std::string pub_bin;
  std::string sec_bin;
  ASSERT_TRUE(make_curve_keypair(pub_bin, sec_bin));

  arqmq::SocketStack stack;
  ASSERT_FALSE(stack.start());
  stack.set_curve_identity(pub_bin, sec_bin);
  stack.set_allow_connection([](const std::string&, const std::string&) { return arqmq::CurvePeerAllow::ServiceNode; });
  ASSERT_TRUE(stack.curve_zap_configured());

  EXPECT_FALSE(stack.bind_curve("tcp://127.0.0.1:0"));
  EXPECT_FALSE(stack.last_curve_endpoint().empty());
  stack.peers().note_peer(make_pubkey('Z'), "tcp://127.0.0.1:1", true);
  EXPECT_EQ(1u, stack.peers().size());
  stack.stop();
}

TEST(arqmq_curve_zap, curve_peer_send_delivers_vote_ob)
{
  std::string server_pub;
  std::string server_sec;
  std::string client_pub;
  std::string client_sec;
  ASSERT_TRUE(make_curve_keypair(server_pub, server_sec));
  ASSERT_TRUE(make_curve_keypair(client_pub, client_sec));

  std::mutex mu;
  std::condition_variable cv;
  bool got = false;
  std::string got_payload;

  arqmq::SocketStack server;
  ASSERT_FALSE(server.start());
  server.set_curve_identity(server_pub, server_sec);
  server.set_allow_connection([&](const std::string&, const std::string& pk) {
    return pk == client_pub ? arqmq::CurvePeerAllow::ServiceNode : arqmq::CurvePeerAllow::Denied;
  });
  server.register_handler("vote_ob", arqmq::CategoryAcl::ServiceNode, [&](const arqmq::InboundRequest& req) {
    {
      std::lock_guard<std::mutex> lock{mu};
      got_payload = req.payload;
      got = true;
    }
    cv.notify_one();
    return std::string{"ok"};
  });
  ASSERT_FALSE(server.bind_curve("tcp://127.0.0.1:0"));
  const std::string endpoint = server.last_curve_endpoint();
  ASSERT_FALSE(endpoint.empty());

  arqmq::SocketStack client;
  ASSERT_FALSE(client.start());
  client.set_curve_identity(client_pub, client_sec);
  client.set_allow_connection([](const std::string&, const std::string&) { return arqmq::CurvePeerAllow::Denied; });
  client.peers().note_peer(server_pub, endpoint, true);

  EXPECT_FALSE(client.send_to_peer(server_pub, "vote_ob", "vote-payload"));

  {
    std::unique_lock<std::mutex> lock{mu};
    ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(3), [&] { return got; }));
  }
  EXPECT_EQ("vote-payload", got_payload);

  client.stop();
  server.stop();
}

TEST(arqmq_curve_zap, native_mesh_blocker_awaits_vote_ob_parity)
{
  EXPECT_FALSE(arqmq::native_mesh_implementation_ready());
  EXPECT_STREQ("vote-ob-parity-unverified", arqmq::native_mesh_blocker());
  EXPECT_TRUE(arqmq::peer_mesh_is_snnetwork());
}

TEST(arqmq_curve_zap, shadow_relay_opt_in_default_off)
{
  EXPECT_FALSE(arqmq::native_mesh_shadow_relay_enabled());
  arqmq::set_native_mesh_shadow_relay_enabled(true);
  EXPECT_TRUE(arqmq::native_mesh_shadow_relay_enabled());
  arqmq::set_native_mesh_shadow_relay_enabled(false);
  EXPECT_FALSE(arqmq::native_mesh_shadow_relay_enabled());
  // No active stack / not configured → no-op.
  arqmq::shadow_send_to_peer(make_pubkey('S'), "vote_ob", "x", "tcp://127.0.0.1:1");
}

TEST(arqmq_curve_zap, shadow_dual_write_delivers_vote_ob_via_facade)
{
  std::string server_pub;
  std::string server_sec;
  std::string client_pub;
  std::string client_sec;
  ASSERT_TRUE(make_curve_keypair(server_pub, server_sec));
  ASSERT_TRUE(make_curve_keypair(client_pub, client_sec));

  std::mutex mu;
  std::condition_variable cv;
  bool got = false;
  std::string got_payload;

  arqmq::SocketStack server;
  ASSERT_FALSE(server.start());
  server.set_curve_identity(server_pub, server_sec);
  server.set_allow_connection([&](const std::string&, const std::string& pk) {
    return pk == client_pub ? arqmq::CurvePeerAllow::ServiceNode : arqmq::CurvePeerAllow::Denied;
  });
  server.register_handler("vote_ob", arqmq::CategoryAcl::ServiceNode, [&](const arqmq::InboundRequest& req) {
    {
      std::lock_guard<std::mutex> lock{mu};
      got_payload = req.payload;
      got = true;
    }
    cv.notify_one();
    return std::string{"ok"};
  });
  ASSERT_FALSE(server.bind_curve("tcp://127.0.0.1:0"));
  const std::string endpoint = server.last_curve_endpoint();
  ASSERT_FALSE(endpoint.empty());

  EXPECT_FALSE(arqmq::shutdown());
  EXPECT_FALSE(arqmq::init(arqmq::Config{arqmq::Backend::ArqMq, arqmq::CategoryAcl::ServiceNode}));
  auto* client = arqmq::active_socket_stack();
  ASSERT_NE(nullptr, client);
  arqmq::configure_mesh_shadow(*client, client_pub, client_sec,
                               [](const std::string&, const std::string&) { return arqmq::CurvePeerAllow::Denied; });

  arqmq::set_native_mesh_shadow_relay_enabled(true);
  arqmq::note_live_mesh_relay("vote_ob");
  arqmq::shadow_send_to_peer(server_pub, "vote_ob", "dual-write-payload", endpoint);

  {
    std::unique_lock<std::mutex> lock{mu};
    ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(3), [&] { return got; }));
  }
  EXPECT_EQ("dual-write-payload", got_payload);

  const auto stats = arqmq::native_mesh_shadow_stats();
  EXPECT_EQ(1u, stats.live_relays);
  EXPECT_EQ(1u, stats.vote_ob_live);
  EXPECT_GE(stats.ok, 1u);
  EXPECT_GE(stats.vote_ob_shadow_ok, 1u);
  EXPECT_GE(arqmq::native_mesh_shadow_ok_rate_bps(), 1u);
  EXPECT_FALSE(arqmq::native_mesh_shadow_parity_sample_ok(32, 9500)); // sample too small
  EXPECT_TRUE(arqmq::native_mesh_shadow_parity_sample_ok(1, 9000));

  arqmq::set_native_mesh_shadow_relay_enabled(false);
  EXPECT_FALSE(arqmq::shutdown());
  server.stop();
}

TEST(arqmq_curve_zap, parity_sample_requires_vote_ob_volume)
{
  arqmq::set_native_mesh_shadow_relay_enabled(true); // resets counters
  EXPECT_FALSE(arqmq::native_mesh_shadow_parity_sample_ok(1, 1));
  arqmq::note_live_mesh_relay("ping");
  EXPECT_FALSE(arqmq::native_mesh_shadow_parity_sample_ok(1, 1)); // not vote_ob
  arqmq::set_native_mesh_shadow_relay_enabled(false);
}
