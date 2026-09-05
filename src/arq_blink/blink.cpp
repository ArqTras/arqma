// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "blink.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace arq_blink {
std::size_t min_signatures_for_quorum(const std::size_t quorum_size) noexcept
{
  if (quorum_size == 0)
    return 0;
  // ceil(2n/3) → 7 of 10.
  return (quorum_size * 2 + 2) / 3;
}

std::size_t leader_index(const std::uint64_t height, const std::size_t sn_count) noexcept
{
  if (sn_count == 0)
    return 0;
  crypto::hash hashed{};
  crypto::cn_fast_hash(&height, sizeof(height), hashed);
  std::uint64_t seed = 0;
  std::memcpy(&seed, hashed.data, sizeof(seed));
  return static_cast<std::size_t>(seed % sn_count);
}

std::vector<std::size_t> quorum_indices(const std::uint64_t height, const std::size_t sn_count,
                                        const std::size_t quorum_size)
{
  std::vector<std::size_t> out;
  if (sn_count == 0 || quorum_size == 0)
    return out;
  const std::size_t take = sn_count < quorum_size ? sn_count : quorum_size;
  out.reserve(take);
  const std::size_t lead = leader_index(height, sn_count);
  for (std::size_t i = 0; i < take; ++i)
    out.push_back((lead + i) % sn_count);
  return out;
}

crypto::hash tx_round_hash(const std::uint64_t height, const crypto::hash& txid)
{
  std::array<char, sizeof(height) + sizeof(txid)> buf{};
  std::memcpy(buf.data(), &height, sizeof(height));
  std::memcpy(buf.data() + sizeof(height), &txid, sizeof(txid));
  return crypto::cn_fast_hash(buf.data(), buf.size());
}

crypto::signature sign_tx(const crypto::hash& hashed, const crypto::public_key& pub, const crypto::secret_key& sec)
{
  crypto::signature sig{};
  crypto::generate_signature(hashed, pub, sec, sig);
  return sig;
}

bool check_tx_signature(const crypto::hash& hashed, const crypto::public_key& pub, const crypto::signature& sig)
{
  return crypto::check_signature(hashed, pub, sig);
}

const char* blocker() noexcept
{
  return "none";
}

void Collector::reset(const std::uint64_t height, const crypto::hash& txid)
{
  std::lock_guard<std::mutex> lock{mu_};
  height_ = height;
  txid_ = txid;
  votes_.clear();
}

bool Collector::add_vote(const Vote& vote, const crypto::hash& hashed)
{
  if (!check_tx_signature(hashed, vote.pub, vote.signature))
    return false;
  std::lock_guard<std::mutex> lock{mu_};
  for (const auto& existing : votes_) {
    if (existing.validator_index == vote.validator_index)
      return std::memcmp(&existing.signature, &vote.signature, sizeof(crypto::signature)) == 0;
  }
  votes_.push_back(vote);
  std::sort(votes_.begin(), votes_.end(),
            [](const Vote& a, const Vote& b) { return a.validator_index < b.validator_index; });
  return true;
}

std::uint64_t Collector::height() const
{
  std::lock_guard<std::mutex> lock{mu_};
  return height_;
}

crypto::hash Collector::txid() const
{
  std::lock_guard<std::mutex> lock{mu_};
  return txid_;
}

std::size_t Collector::signature_count() const
{
  std::lock_guard<std::mutex> lock{mu_};
  return votes_.size();
}

bool Collector::majority_ok(const std::size_t quorum_size) const
{
  std::lock_guard<std::mutex> lock{mu_};
  return votes_.size() >= min_signatures_for_quorum(quorum_size);
}

Collector& collector()
{
  static Collector instance;
  return instance;
}
} // namespace arq_blink
