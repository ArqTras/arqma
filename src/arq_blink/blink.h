// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "crypto/crypto.h"
#include "crypto/hash.h"

namespace arq_blink {
/// Local collector for Blink-style quorum signatures (7 of 10). There is no
/// Arq-Net wire command yet; `get_blink_status` reports this collector only.
/// It does not pre-confirm transactions on the network and does not replace
/// Pulse or RandomARQ.
inline constexpr std::size_t k_quorum_size = 10;

std::size_t min_signatures_for_quorum(std::size_t quorum_size) noexcept;
std::size_t leader_index(std::uint64_t height, std::size_t sn_count) noexcept;
std::vector<std::size_t> quorum_indices(std::uint64_t height, std::size_t sn_count,
                                        std::size_t quorum_size = k_quorum_size);

crypto::hash tx_round_hash(std::uint64_t height, const crypto::hash& txid);
crypto::signature sign_tx(const crypto::hash& hashed, const crypto::public_key& pub, const crypto::secret_key& sec);
bool check_tx_signature(const crypto::hash& hashed, const crypto::public_key& pub, const crypto::signature& sig);

const char* blocker() noexcept;

struct Vote
{
  std::uint32_t validator_index = 0;
  crypto::public_key pub{};
  crypto::signature signature{};
};

class Collector
{
public:
  void reset(std::uint64_t height, const crypto::hash& txid);
  bool add_vote(const Vote& vote, const crypto::hash& hashed);
  std::uint64_t height() const;
  crypto::hash txid() const;
  std::size_t signature_count() const;
  bool majority_ok(std::size_t quorum_size = k_quorum_size) const;

private:
  mutable std::mutex mu_;
  std::uint64_t height_ = 0;
  crypto::hash txid_{};
  std::vector<Vote> votes_;
};

Collector& collector();
} // namespace arq_blink
