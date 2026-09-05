// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cryptonote_config.h"

namespace service_nodes
{
namespace pulse
{
/// HF20: PoW miners and service-node Pulse rounds coexist (hybrid POSPOW).
/// HF21: new-style only — Pulse SN block production + native mesh exclusive.
inline constexpr uint8_t k_hf_hybrid = static_cast<uint8_t>(cryptonote::network_version_20);
inline constexpr uint8_t k_hf_exclusive = static_cast<uint8_t>(cryptonote::network_version_21);

/// Oxen-style Pulse quorum width (leader + validators).
inline constexpr size_t k_quorum_size = 11;

enum class SnMode : uint8_t
{
  Legacy = 0,     ///< < HF20: miner PoW + SNNetwork mesh
  Hybrid = 1,     ///< HF20: PoW + Pulse rounds + dual mesh
  Exclusive = 2,  ///< HF21: Pulse + native mesh only (when implementation ready)
};

const char* to_string(SnMode mode) noexcept;

SnMode sn_operating_mode(uint8_t hard_fork_version) noexcept;

/// True from HF20 (hybrid window and exclusive).
bool hybrid_sn_permitted(uint8_t hard_fork_version) noexcept;

/// True from HF21 (operators must be on the new path).
bool exclusive_sn_required(uint8_t hard_fork_version) noexcept;

/// Pulse round leader among `sn_count` active service nodes for `height`.
/// Deterministic (cn_fast_hash of height). Returns 0 when sn_count == 0.
size_t leader_index(uint64_t height, size_t sn_count) noexcept;

/// Leader plus the next `quorum_size-1` indices, wrapping. Empty if sn_count == 0.
std::vector<size_t> quorum_indices(uint64_t height, size_t sn_count,
                                   size_t quorum_size = k_quorum_size);

/// False until a Pulse block producer + signature path is wired. While false,
/// `pow_required_for_block` stays true even at HF21 so the chain cannot stall.
bool pow_replacement_ready() noexcept;

/// Miner RandomARQ is required unless exclusive mode AND replacement is ready.
bool pow_required_for_block(uint8_t hard_fork_version) noexcept;

const char* blocker() noexcept;
} // namespace pulse
} // namespace service_nodes
