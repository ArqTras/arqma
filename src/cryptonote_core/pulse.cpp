// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "pulse.h"

#include "crypto/hash.h"

#include <cstring>

namespace service_nodes
{
namespace pulse
{
namespace
{
// 0 = leader/quorum helpers only; PoW still required on every block.
// Flip with a working Pulse producer before HF21 exclusive can drop RandomARQ.
constexpr int k_pulse_pow_stage = 0;
} // namespace

const char* to_string(const SnMode mode) noexcept
{
  switch (mode)
  {
  case SnMode::Legacy:
    return "legacy";
  case SnMode::Hybrid:
    return "hybrid";
  case SnMode::Exclusive:
    return "exclusive";
  }
  return "legacy";
}

SnMode sn_operating_mode(const uint8_t hard_fork_version) noexcept
{
  if (hard_fork_version >= k_hf_exclusive)
    return SnMode::Exclusive;
  if (hard_fork_version >= k_hf_hybrid)
    return SnMode::Hybrid;
  return SnMode::Legacy;
}

bool hybrid_sn_permitted(const uint8_t hard_fork_version) noexcept
{
  return hard_fork_version >= k_hf_hybrid;
}

bool exclusive_sn_required(const uint8_t hard_fork_version) noexcept
{
  return hard_fork_version >= k_hf_exclusive;
}

size_t leader_index(const uint64_t height, const size_t sn_count) noexcept
{
  if (sn_count == 0)
    return 0;
  crypto::hash hashed{};
  crypto::cn_fast_hash(&height, sizeof(height), hashed);
  uint64_t seed = 0;
  std::memcpy(&seed, hashed.data, sizeof(seed));
  return static_cast<size_t>(seed % sn_count);
}

std::vector<size_t> quorum_indices(const uint64_t height, const size_t sn_count, const size_t quorum_size)
{
  std::vector<size_t> out;
  if (sn_count == 0 || quorum_size == 0)
    return out;
  const size_t take = sn_count < quorum_size ? sn_count : quorum_size;
  out.reserve(take);
  const size_t lead = leader_index(height, sn_count);
  for (size_t i = 0; i < take; ++i)
    out.push_back((lead + i) % sn_count);
  return out;
}

bool pow_replacement_ready() noexcept
{
  return k_pulse_pow_stage >= 1;
}

bool pow_required_for_block(const uint8_t hard_fork_version) noexcept
{
  return !(exclusive_sn_required(hard_fork_version) && pow_replacement_ready());
}

const char* blocker() noexcept
{
  if (k_pulse_pow_stage < 1)
    return "pulse-block-producer-unwired";
  return "none";
}
} // namespace pulse
} // namespace service_nodes
