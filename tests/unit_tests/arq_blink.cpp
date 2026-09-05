// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "gtest/gtest.h"

#include "arq_blink/blink.h"

TEST(arq_blink, majority_is_seven_of_ten)
{
  EXPECT_EQ(7u, arq_blink::min_signatures_for_quorum(arq_blink::k_quorum_size));
  EXPECT_EQ(0u, arq_blink::min_signatures_for_quorum(0));
}

TEST(arq_blink, quorum_rotates_from_leader)
{
  const auto q = arq_blink::quorum_indices(100, 20);
  ASSERT_EQ(arq_blink::k_quorum_size, q.size());
  EXPECT_EQ(arq_blink::leader_index(100, 20), q.front());
  EXPECT_EQ(std::string("blink-wire-not-connected"), arq_blink::blocker());
}

TEST(arq_blink, sign_verify_and_collector_majority)
{
  crypto::public_key pub{};
  crypto::secret_key sec{};
  crypto::generate_keys(pub, sec);
  crypto::hash txid{};
  txid.data[0] = 7;
  const auto hashed = arq_blink::tx_round_hash(42, txid);
  const auto sig = arq_blink::sign_tx(hashed, pub, sec);
  EXPECT_TRUE(arq_blink::check_tx_signature(hashed, pub, sig));

  arq_blink::Collector col;
  col.reset(42, txid);
  for (std::uint32_t i = 0; i < 7; ++i) {
    arq_blink::Vote vote{};
    vote.validator_index = i;
    vote.pub = pub;
    vote.signature = sig;
    EXPECT_TRUE(col.add_vote(vote, hashed));
  }
  EXPECT_EQ(7u, col.signature_count());
  EXPECT_TRUE(col.majority_ok());
  EXPECT_FALSE(arq_blink::collector().majority_ok());
}
