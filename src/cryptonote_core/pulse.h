// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_config.h"

namespace service_nodes
{
namespace pulse
{
/// HF20+: PoW miners and service-node Pulse rounds coexist (hybrid POSPOW).
/// HF21: native mesh exclusive intent; RandomARQ stays required (no PoW-off).
inline constexpr uint8_t k_hf_hybrid = static_cast<uint8_t>(cryptonote::network_version_20);
inline constexpr uint8_t k_hf_exclusive = static_cast<uint8_t>(cryptonote::network_version_21);

/// Oxen-style Pulse quorum width (leader + validators).
inline constexpr size_t k_quorum_size = 11;

/// Seconds per Pulse fallback round. 8 rounds cover one 120s block target.
inline constexpr uint64_t k_round_window_seconds = 15;
inline constexpr uint8_t k_max_round = 7;

/// Not in quorum / not a local participant.
inline constexpr size_t k_not_in_quorum = std::numeric_limits<size_t>::max();

enum class SnMode : uint8_t
{
  Legacy = 0,     ///< < HF20: miner PoW + SNNetwork mesh
  Hybrid = 1,     ///< HF20: PoW + Pulse rounds + dual mesh
  Exclusive = 2,  ///< HF21: native mesh exclusive intent; blocks stay hybrid PoW + Pulse
};

const char* to_string(SnMode mode) noexcept;

SnMode sn_operating_mode(uint8_t hard_fork_version) noexcept;

/// True from HF20 (hybrid window and exclusive).
bool hybrid_sn_permitted(uint8_t hard_fork_version) noexcept;

/// True from HF21 (operators must be on the new path).
bool exclusive_sn_required(uint8_t hard_fork_version) noexcept;

/// Pulse round leader among `sn_count` active service nodes for `height`.
/// Deterministic (cn_fast_hash of height), then rotated by `round`.
/// Returns 0 when sn_count == 0.
size_t leader_index(uint64_t height, size_t sn_count, uint8_t round = 0) noexcept;

/// Leader plus the next `quorum_size-1` indices, wrapping. Empty if sn_count == 0.
std::vector<size_t> quorum_indices(uint64_t height, size_t sn_count, uint8_t round = 0,
                                   size_t quorum_size = k_quorum_size);

/// Wait-window round from parent timestamp → now. Caps at `k_max_round`.
/// Unknown parent (`0`) stays round 0 so alt/missing timestamps do not jump to the last window.
uint8_t round_from_timestamps(uint64_t parent_timestamp, uint64_t now) noexcept;

/// True when `signatures` meets `min_signatures_for_quorum(quorum_size)`.
bool majority_reached(size_t signatures, size_t quorum_size) noexcept;

/// Majority threshold: max(1, 2/3 of quorum). 11 → 7.
size_t min_signatures_for_quorum(size_t quorum_size) noexcept;

/// Slot of `pub` in this height/round quorum (0 = leader), or `k_not_in_quorum`.
size_t validator_slot(uint64_t height, const std::vector<crypto::public_key>& active_pubs,
                      const crypto::public_key& pub, uint8_t round = 0);

/// Ordered quorum public keys for `height`/`round` (leader first).
std::vector<crypto::public_key> quorum_pubkeys(uint64_t height,
                                               const std::vector<crypto::public_key>& active_pubs,
                                               uint8_t round = 0);

/// Hash signed by quorum members: little-endian height || round || prev_id.
crypto::hash round_hash(uint64_t height, const crypto::hash& prev_id, uint8_t round);

crypto::signature sign_round(const crypto::hash& hashed, const crypto::public_key& pub,
                             const crypto::secret_key& sec);

bool check_round_signature(const crypto::hash& hashed, const crypto::public_key& pub,
                           const crypto::signature& sig);

/// If `pub` is in the quorum for `round`, fill `out` with a single local vote.
bool try_local_round(uint64_t height, const crypto::hash& prev_id, uint8_t round,
                     const crypto::public_key& pub, const crypto::secret_key& sec,
                     const std::vector<crypto::public_key>& active_pubs,
                     cryptonote::tx_extra_pulse_round& out);

/// Prepare the in-memory collector for this window and, if `pub` is in the
/// quorum, add a local vote. When `relay` is true, gossip even if the vote was
/// already collected (retransmit for late quorum peers). Does not waive RandomARQ.
bool participate_round(uint64_t height, const crypto::hash& prev_id, uint8_t round,
                       const crypto::public_key& pub, const crypto::secret_key& sec,
                       const std::vector<crypto::public_key>& active_pubs,
                       bool relay = true);

/// Structural + signature checks. `min_signatures` is 1 while Pulse extra is
/// optional (hybrid PoW policy). Majority is only required if PoW is ever dropped.
bool verify_round(const cryptonote::tx_extra_pulse_round& extra, uint64_t height,
                  const crypto::hash& prev_id, size_t active_sn_count,
                  const std::vector<crypto::public_key>& quorum_keys,
                  size_t min_signatures);

/// Packed vote used on Arq-Net `pulse_rnd` (version || height || round || leader || index || prev || sig).
struct RelayVote
{
  uint64_t height = 0;
  uint8_t round = 0;
  uint32_t leader_index = 0;
  uint32_t validator_index = 0;
  crypto::hash prev_id{};
  crypto::signature signature{};
};

inline constexpr uint8_t k_relay_vote_version = 1;
inline constexpr size_t k_relay_vote_bytes =
    1 + sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + crypto::HASH_SIZE +
    sizeof(crypto::signature);

bool encode_relay_vote(const RelayVote& vote, std::string& out);
bool decode_relay_vote(std::string_view blob, RelayVote& out);

/// In-memory Pulse round: accumulates verified votes for one (height, prev, round).
/// Votes for an older round of the same height/prev are dropped; a newer round replaces the set.
class RoundCollector
{
public:
  void prepare(uint64_t height, const crypto::hash& prev_id, uint8_t round, uint32_t leader_index);
  /// Returns true when `vote` is new and valid. Optionally invokes the relay hook.
  bool add_vote(const RelayVote& vote, const std::vector<crypto::public_key>& quorum_keys,
                size_t active_sn_count, bool relay_if_new = true);
  bool snapshot(cryptonote::tx_extra_pulse_round& out) const;
  bool has_vote(uint32_t validator_index) const;
  uint64_t height() const;
  uint8_t round() const;
  size_t signature_count() const;
  /// Drop votes for heights that have already been produced (`height` is the next block).
  void discard_below(uint64_t height);
  void clear();

private:
  mutable std::mutex m_mu;
  cryptonote::tx_extra_pulse_round m_extra{};
  crypto::hash m_prev_id{};
};

RoundCollector& collector();

/// Daemon Arq-Net sets this so a newly accepted vote is gossiped to the Pulse quorum.
void set_relay_new_vote(void (*fn)(const RelayVote&));

/// Always false under the hybrid policy: Pulse extra is optional and does not
/// replace miner RandomARQ. Stage 3 (PoW-off) is reserved and not scheduled.
bool pow_replacement_ready() noexcept;

/// Miner RandomARQ is always required (HF20/HF21 hybrid). Pulse signatures may
/// coexist on the miner extra; they do not waive the PoW check.
bool pow_required_for_block(uint8_t hard_fork_version) noexcept;

const char* blocker() noexcept;
} // namespace pulse
} // namespace service_nodes
