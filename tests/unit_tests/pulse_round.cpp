// Copyright (c) 2026, The Arqma Network
//
// Pulse round index and PoS cumulative-diff sanity checks (scoped; no Blockchain mock).

#include <chrono>
#include <vector>

#include "gtest/gtest.h"

#include "common/arqma_pulse_fork.h"
#include "cryptonote_basic/difficulty.h"
#include "cryptonote_core/pulse.h"

using namespace std::chrono_literals;

namespace
{
constexpr std::chrono::seconds kPulseSlot(static_cast<int>(arqma::pulse_fork::PULSE_ROUND_TIMEOUT_SEC));
}

TEST(PulseRound, convert_time_round_zero_before_or_at_r0)
{
  using clock = std::chrono::system_clock;
  const auto r0 = clock::from_time_t(1'500'000'000);

  uint8_t round_out = 9;
  ASSERT_TRUE(cryptonote::pulse::convert_time_to_round(cryptonote::MAINNET, r0 - 500s, r0, &round_out));
  EXPECT_EQ(round_out, 0);

  ASSERT_TRUE(cryptonote::pulse::convert_time_to_round(cryptonote::MAINNET, r0, r0, &round_out));
  EXPECT_EQ(round_out, 0);
}

TEST(PulseRound, convert_time_round_increments_per_slot)
{
  using clock = std::chrono::system_clock;
  const auto r0 = clock::from_time_t(1'500'000'000);
  uint8_t round_out = 0;

  ASSERT_TRUE(cryptonote::pulse::convert_time_to_round(cryptonote::MAINNET, r0 + kPulseSlot - 1s, r0, &round_out));
  EXPECT_EQ(round_out, 0);

  ASSERT_TRUE(cryptonote::pulse::convert_time_to_round(cryptonote::MAINNET, r0 + kPulseSlot, r0, &round_out));
  EXPECT_EQ(round_out, 1);
}

TEST(PulseRound, convert_time_round_255_inclusive)
{
  using clock = std::chrono::system_clock;
  const auto r0 = clock::from_time_t(1'500'000'000);
  const auto t = r0 + 255 * kPulseSlot;
  uint8_t round_out = 0;
  ASSERT_TRUE(cryptonote::pulse::convert_time_to_round(cryptonote::MAINNET, t, r0, &round_out));
  EXPECT_EQ(round_out, 255);
}

TEST(PulseRound, convert_time_rejects_round_overflow)
{
  using clock = std::chrono::system_clock;
  const auto r0 = clock::from_time_t(1'500'000'000);
  const auto t = r0 + 256 * kPulseSlot;
  uint8_t round_out = 0;
  EXPECT_FALSE(cryptonote::pulse::convert_time_to_round(cryptonote::MAINNET, t, r0, &round_out));
}

TEST(PulseMergeBitset, high_indices_allowed_without_header_bits)
{
  EXPECT_TRUE(cryptonote::pulse::merge_vote_matches_validator_bitset(16, 0));
  EXPECT_TRUE(cryptonote::pulse::merge_vote_matches_validator_bitset(19, 0));
}

TEST(PulseMergeBitset, low_indices_require_participation_bit)
{
  EXPECT_FALSE(cryptonote::pulse::merge_vote_matches_validator_bitset(0, 0));
  uint16_t const mask = static_cast<uint16_t>(1u << 3);
  EXPECT_TRUE(cryptonote::pulse::merge_vote_matches_validator_bitset(3, mask));
  EXPECT_FALSE(cryptonote::pulse::merge_vote_matches_validator_bitset(4, mask));
}

TEST(PulseDifficulty, pulse_pos_differs_from_v16_on_same_window)
{
  std::vector<uint64_t> timestamps(60);
  std::vector<cryptonote::difficulty_type> cumulative(60);
  const uint64_t t_base = 2'100'000'000;
  for (size_t i = 0; i < timestamps.size(); ++i)
  {
    timestamps[i] = t_base + static_cast<uint64_t>(i * 120);
    cumulative[i] = static_cast<cryptonote::difficulty_type>(100000 + i * 1000);
  }

  cryptonote::difficulty_type const d_pulse = cryptonote::next_difficulty_pulse_pos(timestamps, cumulative);
  cryptonote::difficulty_type const d_v16 = cryptonote::next_difficulty_v16(timestamps, cumulative);
  ASSERT_GT(d_pulse, 0);
  ASSERT_GT(d_v16, 0);
  EXPECT_NE(d_pulse, d_v16);
}
