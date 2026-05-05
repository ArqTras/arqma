// Copyright (c) 2026, The Arqma Network
//
// Pulse round timing helpers — math aligned with oxen-core (get_round_timings / convert_time_to_round),
// parameterized via arqma::pulse_fork timings and Arqma HF v20 scheduling.

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include "cryptonote_config.h"

namespace cryptonote
{

class Blockchain;

namespace pulse
{

struct round_timings
{
  std::chrono::system_clock::time_point genesis_timestamp{};
  std::chrono::system_clock::time_point prev_timestamp{};
  std::chrono::system_clock::time_point ideal_timestamp{};
  std::chrono::system_clock::time_point r0_timestamp{};
  std::chrono::system_clock::time_point miner_fallback_timestamp{};
};

/**
 * Timestamps for the Pulse round window of block at \p block_height given the previous block's on-chain time.
 * genesis_timestamp anchors to the last block before the first ideal v20 (PoS) height (parent of first Pulse block).
 */
std::optional<round_timings> get_round_timings(Blockchain const &blockchain, uint64_t block_height, uint64_t prev_timestamp);

/** Map wall clock to round index 0..255 (returns false if computed round > 255). */
bool convert_time_to_round(
    network_type nettype,
    std::chrono::system_clock::time_point const &time,
    std::chrono::system_clock::time_point const &r0_timestamp,
    uint8_t *round_out);

} // namespace pulse
} // namespace cryptonote
