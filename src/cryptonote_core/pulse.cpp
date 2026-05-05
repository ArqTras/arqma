// Copyright (c) 2026, The Arqma Network

#include "pulse.h"

#include <algorithm>
#include <chrono>
#include <limits>

#include "blockchain.h"
#include "common/arqma_pulse_fork.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

namespace cryptonote
{
namespace pulse
{
namespace
{
using clock = std::chrono::system_clock;

uint64_t first_ideal_pos_block_height(Blockchain const &blockchain)
{
  uint64_t h = blockchain.get_earliest_ideal_height_for_version(cryptonote::network_version_20_pos);
  constexpr uint64_t kMax = std::numeric_limits<uint64_t>::max();
  if (h == kMax || h == 0)
    h = arqma::pulse_fork::planned_fork_height_for_net(blockchain.nettype());
  return h;
}

} // namespace

std::optional<round_timings> get_round_timings(Blockchain const &blockchain, uint64_t block_height, uint64_t prev_timestamp)
{
  if (!arqma::pulse_fork::fork_active(blockchain.nettype()))
    return std::nullopt;

  uint64_t const first_pos_h = first_ideal_pos_block_height(blockchain);
  if (first_pos_h == 0)
    return std::nullopt;

  if (blockchain.get_current_blockchain_height() < first_pos_h)
    return std::nullopt;

  uint64_t const anchor_height = first_pos_h - 1;
  crypto::hash const ph = blockchain.get_block_id_by_height(anchor_height);
  block anchor{};
  if (!blockchain.get_block_by_hash(ph, anchor))
    return std::nullopt;

  unsigned const T = arqma::pulse_fork::PULSE_TARGET_BLOCK_TIME_SEC;
  unsigned const adj = arqma::pulse_fork::PULSE_MAX_START_ADJUSTMENT_SEC;
  unsigned const slot = arqma::pulse_fork::PULSE_ROUND_TIMEOUT_SEC;

  round_timings times{};
  uint64_t const anchor_chain_height = cryptonote::get_block_height(anchor);
  uint64_t const delta_height = block_height > anchor_chain_height ? block_height - anchor_chain_height : 0;

  times.genesis_timestamp = clock::from_time_t(anchor.timestamp);
  times.prev_timestamp = clock::from_time_t(prev_timestamp);
  times.ideal_timestamp = times.genesis_timestamp + std::chrono::seconds(T) * static_cast<int64_t>(delta_height);

  times.r0_timestamp = std::clamp(
      times.ideal_timestamp,
      times.prev_timestamp + std::chrono::seconds(static_cast<int>(T)) - std::chrono::seconds(static_cast<int>(adj)),
      times.prev_timestamp + std::chrono::seconds(static_cast<int>(T)) + std::chrono::seconds(static_cast<int>(adj)));

  times.miner_fallback_timestamp = times.r0_timestamp + std::chrono::seconds(static_cast<int>(slot)) * 255;
  return times;
}

bool convert_time_to_round(
    network_type nettype,
    std::chrono::system_clock::time_point const &time,
    std::chrono::system_clock::time_point const &r0_timestamp,
    uint8_t *round_out)
{
  (void)nettype;
  using namespace std::chrono;
  seconds const time_since_round_started = time <= r0_timestamp ? 0s : duration_cast<seconds>(time - r0_timestamp);
  size_t const result_usize = static_cast<size_t>(
      time_since_round_started.count() / static_cast<int64_t>(arqma::pulse_fork::PULSE_ROUND_TIMEOUT_SEC));
  if (round_out)
    *round_out = static_cast<uint8_t>(result_usize);
  return result_usize <= 255;
}

} // namespace pulse
} // namespace cryptonote
