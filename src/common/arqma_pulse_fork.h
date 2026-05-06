// Copyright (c) 2026, The Arqma Network
//
// PoW/PoS Hybrid (Pulse slots) — active per-network when hardfork.cpp schedules network_version_20 (stagenet on by default;
// mainnet off until the v20 row is uncommented). Optional FORK_ACTIVE overrides all nets when true.
//
// Repository convention: all Git commit messages must be written in English.

#pragma once

#include <cstddef>
#include <cstdint>

#include "cryptonote_basic/hardfork.h"
#include "cryptonote_config.h"

#ifndef ARQMA_ANNOUNCE_PLANNED_POS_TRANSITION
/** Log a one-time notice about the planned PoW/PoS Hybrid transition at daemon startup (where not yet active). */
#define ARQMA_ANNOUNCE_PLANNED_POS_TRANSITION 1
#endif

namespace arqma::pulse_fork {

/** Optional global override to force hybrid logic on every network type (normally false). Mainnet go-live uses uncommented \p network_version_20 in hardfork.cpp; keep this false unless you intentionally override. */
inline constexpr bool FORK_ACTIVE = false;

/**
 * Hybrid consensus rules active when:
 *   - \p FORK_ACTIVE is true (explicit go-live / override), or
 *   - the baked-in HF table for \p net includes \p network_version_20 (stagenet today; mainnet after uncommenting the v20 row in hardfork.cpp).
 */
inline bool fork_active(cryptonote::network_type net) noexcept
{
  if (FORK_ACTIVE)
    return true;
  return cryptonote::HardFork::hard_fork_table_includes_pulse_hybrid(net);
}

inline constexpr char HF_RELEASE_NAME[] = "ARQMA-V11.0.0-PoW-PoS-Hybrid";

/** Mainnet: height where network_version_20 (hybrid) is scheduled; first block there is a PoW slot per alternation rules. */
inline constexpr uint64_t MAINNET_FORK_HEIGHT_PLANNED = 2000000ULL;

/**
 * Stagenet: planned height = last scheduled HF height in stagenet_hard_forks[] + 100.
 * Keeps in sync — update STAGENET_LAST_HF_HEIGHT when adding rows above network_version_20 in hardfork.cpp.
 */
inline constexpr uint64_t STAGENET_LAST_HF_HEIGHT = 220ULL;
inline constexpr uint64_t STAGENET_FORK_HEIGHT_PLANNED = STAGENET_LAST_HF_HEIGHT + 100ULL;

inline constexpr unsigned PULSE_TARGET_BLOCK_TIME_SEC = 60;
/** Oxen-style intra-block round slot size; used with convert_time_to_round and miner_fallback (255 rounds). */
inline constexpr unsigned PULSE_ROUND_TIMEOUT_SEC = 10;
/** Oxen-style clamp: r0 may shift within [prev+T−adj, prev+T+adj] around ideal schedule. */
inline constexpr unsigned PULSE_MAX_START_ADJUSTMENT_SEC = 5;
inline constexpr std::size_t PULSE_QUORUM_VALIDATORS_MIN = 10;
inline constexpr std::size_t PULSE_SIGNATURE_THRESHOLD = 7;

/**
 * Hybrid PoW-slot difficulty: LWMA-v16 base D multiplied linearly by **active** service-node count:
 *   D_pow = max(1, (D * active_sn) / POW_DIFFICULTY_ACTIVE_SN_REFERENCE).
 * At `active_sn == POW_DIFFICULTY_ACTIVE_SN_REFERENCE`, multiplier is 1.0 (fixed reference; tune via HF/governance if needed).
 */
inline constexpr std::uint64_t POW_DIFFICULTY_ACTIVE_SN_REFERENCE = 10;

/**
 * Planned Pulse/PoS quorums reuse service-node operators; those nodes already require a running storage server that
 * calls arqmad's storage_server_ping (Arqma Pulse / SN pairing policy). Compatible stack: github.com ArqTras fork arqma-storage-server plus arqmad.
 */
inline constexpr bool POS_EXPECTS_STORAGE_SERVER_FOR_SERVICE_NODES = true;

/** Startup hint when this daemon acts as an SN operator (paired process for uptime proofs). */
inline constexpr char SN_POS_STORAGE_PAIRING_HINT[] =
    "PoW/PoS Hybrid (Pulse slots) expects the same pairing as today: keep arqma-storage-server (ArqTras fork) reachable and pinging "
    "this daemon's RPC (--arqmad-rpc-port in storage-server, sn_bind/storage port on daemon).";

/** Shown when RPC refuses PoW mining (e.g. Pulse slot or legacy full-PoS-disabled path; HF >= network_version_20 rules). */
inline constexpr char POW_MINING_DISABLED_RPC_MESSAGE[] =
    "PoW mining is not available for this block — use Pulse block production on Pulse slots (PoW/PoS Hybrid consensus)";

inline constexpr char NOTICE_LOG[] =
    "Planned PoW/PoS Hybrid HF ARQMA-V11.0.0 (not active on this network in default mainnet builds): "
    "mainnet transition height 2000000; stagenet hybrid from last_HF_height+100 (currently 320). "
    "Pulse slots use 60s target; quorum minimum 10 validators, signature threshold 7; PoW slots alternate by height. "
    "Service-node Pulse quorum follows the Arqma SN model: validators keep arqma-storage-server + arqmad paired.";

/*
 Next steps (from roadmap; incremental port in progress):
 - Wire format: pulse_header + reward + sn_winner_tail (header varints) + pulse_validator_signatures on block when major_version >= network_version_20 (see cryptonote_basic.h, crypto::hash4); JSON via json_object (block dump / RPC tooling).
 - Pulse round scheduling, producer selection, and signer tooling (daemon validates quorum signatures when FORK_ACTIVE).
 - Extend arqnet Pulse (**`pulse_proposal`** / **`pulse_vote`** relay and round state**) beyond point-to-point validate-only paths.
 - When FORK_ACTIVE + HF >= v20, cumulative difficulty uses next_difficulty_pulse_pos (60s LWMA) instead of DIFFICULTY_TARGET_V16. Governance / dev / net coinbase outputs (mainnet GOV_/DEV_/NET_WALLET_ADDRESS) stay on the HF16+ path in construct_miner_tx — do not strip for Pulse without a coordinated consensus + validation change.
 - PoW skip when FORK_ACTIVE + block major_version >= network_version_20; quorum signatures are enforced by verify_pulse_fork_block_rules from service_node_list::block_added / alt_block_added (oxen-core style: after DB accept, in the SNL hook), not alongside PoW skipping.
 - Stagenet/mocknet rehearsal; then uncomment HF rows in hardfork.cpp with real timestamps.
 - Finalize Arqma quorum crypto + Pulse producer scheduling; tighten storage/arqnet liveness checks for validators if spec requires.
 - Optional: checkpoint relay expansion, wallet/RPC alignment, HF_VERSION_PULSE_POS in cryptonote_config.h
 - Coinbase: Pulse header `reward` is the full miner_tx total (miner + SN + gov + dev + net); last three vouts remain gov/dev/net for hf >= 16.
*/

inline constexpr uint64_t planned_fork_height_for_net(cryptonote::network_type net)
{
  switch (net)
  {
    case cryptonote::MAINNET:
      return MAINNET_FORK_HEIGHT_PLANNED;
    case cryptonote::STAGENET:
      return STAGENET_FORK_HEIGHT_PLANNED;
    default:
      return 0;
  }
}

/**
 * Height-based alternation (proposal): from planned fork height `fh` onward, while `fork_active(net)` and
 * `block_major_version >= network_version_20`, every height is exactly one slot type:
 *   PoW   if (block_height - fh) % 2 == 0  — first post-fork v20 block at `fh` is PoW
 *   Pulse if (block_height - fh) % 2 == 1
 * Example: fh=320 → 320 PoW, 321 Pulse, 322 PoW, 323 Pulse, …
 */
inline bool hybrid_pos_pow_alternating_at_height(cryptonote::network_type net, uint64_t block_height,
    uint8_t block_major_version) noexcept
{
  if (!fork_active(net) || block_major_version < cryptonote::network_version_20)
    return false;
  const uint64_t fh = planned_fork_height_for_net(net);
  return fh != 0 && block_height >= fh;
}

/** Next block at \p block_height is a PoW slot (empty Pulse header in v20+ hybrid era). */
inline bool is_pow_slot_at_height(cryptonote::network_type net, uint64_t block_height,
    uint8_t block_major_version) noexcept
{
  if (!hybrid_pos_pow_alternating_at_height(net, block_height, block_major_version))
    return false;
  const uint64_t fh = planned_fork_height_for_net(net);
  return ((block_height - fh) % 2ULL) == 0ULL;
}

/** Next block at \p block_height is a Pulse slot (quorum / pulse_header required). */
inline bool is_pulse_slot_at_height(cryptonote::network_type net, uint64_t block_height,
    uint8_t block_major_version) noexcept
{
  if (!hybrid_pos_pow_alternating_at_height(net, block_height, block_major_version))
    return false;
  const uint64_t fh = planned_fork_height_for_net(net);
  return ((block_height - fh) % 2ULL) == 1ULL;
}

/**
 * When true, PoW block templates must not be produced and mining must not run.
 * In the v20+ hybrid era, PoW is allowed only on PoW slots; Pulse slots use Pulse production.
 * Before v20 at/after fork height, PoW stays disabled (rehearsal path).
 */
inline bool pow_mining_disabled_for_chain(cryptonote::network_type net,
    uint8_t block_major_version_for_next_block,
    uint64_t next_block_height)
{
  if (!fork_active(net))
    return false;

  const uint64_t fh = planned_fork_height_for_net(net);
  if (fh == 0)
    return false;

  if (next_block_height < fh)
    return false;

  if (block_major_version_for_next_block >= cryptonote::network_version_20)
    return !is_pow_slot_at_height(net, next_block_height, block_major_version_for_next_block);

  return true;
}

} // namespace arqma::pulse_fork
