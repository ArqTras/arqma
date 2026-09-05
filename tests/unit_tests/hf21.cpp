// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "gtest/gtest.h"

#include "cryptonote_basic/hardfork.h"
#include "cryptonote_config.h"
#include "cryptonote_core/pulse.h"

TEST(hf21, feature_gate_aligns)
{
  ASSERT_EQ(HF_VERSION_PULSE_EXCLUSIVE, cryptonote::network_version_21);
  ASSERT_EQ(21u, static_cast<unsigned>(cryptonote::network_version_21));
  ASSERT_EQ(service_nodes::pulse::k_hf_exclusive, static_cast<uint8_t>(cryptonote::network_version_21));
}

TEST(hf21, schedules_after_hf20)
{
  ASSERT_EQ(260u,
            cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::STAGENET, cryptonote::network_version_21));
  ASSERT_EQ(1400u,
            cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::TESTNET, cryptonote::network_version_21));
  ASSERT_EQ(MAINNET_HARD_FORK_21_HEIGHT,
            cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::MAINNET, cryptonote::network_version_21));
  ASSERT_EQ(5000000u, MAINNET_HARD_FORK_21_HEIGHT);
  ASSERT_LT(cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::MAINNET, cryptonote::network_version_20),
            cryptonote::HardFork::get_hardcoded_hard_fork_height(cryptonote::MAINNET, cryptonote::network_version_21));
}

TEST(pulse, modes_follow_hard_fork)
{
  using service_nodes::pulse::SnMode;
  EXPECT_EQ(SnMode::Legacy, service_nodes::pulse::sn_operating_mode(19));
  EXPECT_EQ(SnMode::Hybrid, service_nodes::pulse::sn_operating_mode(20));
  EXPECT_EQ(SnMode::Exclusive, service_nodes::pulse::sn_operating_mode(21));
  EXPECT_STREQ("hybrid", service_nodes::pulse::to_string(SnMode::Hybrid));
  EXPECT_STREQ("exclusive", service_nodes::pulse::to_string(SnMode::Exclusive));
  EXPECT_FALSE(service_nodes::pulse::hybrid_sn_permitted(19));
  EXPECT_TRUE(service_nodes::pulse::hybrid_sn_permitted(20));
  EXPECT_FALSE(service_nodes::pulse::exclusive_sn_required(20));
  EXPECT_TRUE(service_nodes::pulse::exclusive_sn_required(21));
}

TEST(pulse, pow_stays_required_until_producer_ready)
{
  EXPECT_FALSE(service_nodes::pulse::pow_replacement_ready());
  EXPECT_TRUE(service_nodes::pulse::pow_required_for_block(19));
  EXPECT_TRUE(service_nodes::pulse::pow_required_for_block(20));
  EXPECT_TRUE(service_nodes::pulse::pow_required_for_block(21));
  EXPECT_STREQ("pulse-block-producer-unwired", service_nodes::pulse::blocker());
}

TEST(pulse, leader_and_quorum_are_deterministic)
{
  EXPECT_EQ(0u, service_nodes::pulse::leader_index(100, 0));
  EXPECT_TRUE(service_nodes::pulse::quorum_indices(100, 0).empty());

  const size_t a = service_nodes::pulse::leader_index(42, 17);
  const size_t b = service_nodes::pulse::leader_index(42, 17);
  const size_t c = service_nodes::pulse::leader_index(43, 17);
  EXPECT_EQ(a, b);
  EXPECT_LT(a, 17u);
  EXPECT_NE(a, c);

  const auto q = service_nodes::pulse::quorum_indices(42, 17);
  ASSERT_EQ(service_nodes::pulse::k_quorum_size, q.size());
  EXPECT_EQ(a, q.front());
  for (size_t i = 1; i < q.size(); ++i)
    EXPECT_EQ((q[i - 1] + 1) % 17, q[i]);
}

TEST(pulse, quorum_shrinks_when_few_service_nodes)
{
  const auto q = service_nodes::pulse::quorum_indices(7, 3);
  ASSERT_EQ(3u, q.size());
}
