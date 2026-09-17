// Copyright (c) 2018 - 2026, The Arqma Network

#include "gtest/gtest.h"

#include "arq_etn/etn_store.h"
#include "arq_etn/etn_types.h"

#include <filesystem>

TEST(arq_etn, attestation_requires_core_fields)
{
  arq_etn::BurnMintAttestation empty{};
  EXPECT_FALSE(arq_etn::attestation_fields_present(empty));

  arq_etn::BurnMintAttestation ok{};
  ok.direction = arq_etn::WrapDirection::Mint;
  ok.arq_txid_hex = "aa";
  ok.arq_amount_atomic = "1";
  ok.eth_txid_hex = "bb";
  ok.warq_amount_atomic = "1";
  ok.eth_chain_id = 1;
  ok.issuer_pubkey_hex = "cc";
  ok.signature_hex = "dd";
  EXPECT_TRUE(arq_etn::attestation_fields_present(ok));
}

TEST(arq_etn, json_roundtrip_and_store)
{
  arq_etn::BurnMintAttestation a{};
  a.direction = arq_etn::WrapDirection::Redeem;
  a.arq_txid_hex = "abc123";
  a.arq_amount_atomic = "42";
  a.eth_txid_hex = "def456";
  a.warq_amount_atomic = "42";
  a.eth_chain_id = 11155111;
  a.arq_height = 9;
  a.issuer_pubkey_hex = "pk";
  a.signature_hex = "sig";
  const auto json = arq_etn::attestation_to_json(a);
  arq_etn::BurnMintAttestation b{};
  ASSERT_TRUE(arq_etn::attestation_from_json(json, b));
  EXPECT_EQ(a.arq_txid_hex, b.arq_txid_hex);
  EXPECT_EQ(arq_etn::WrapDirection::Redeem, b.direction);

  const auto dir = std::filesystem::temp_directory_path() / "arq_etn_test_store";
  std::filesystem::remove_all(dir);
  arq_etn::EtNStore store;
  store.set_data_dir(dir.string());
  store.set_issuer_id("test-issuer");
  store.load();
  ASSERT_TRUE(store.put_attestation(a));
  const auto ids = store.list_attestation_ids();
  ASSERT_EQ(1u, ids.size());
  const auto got = store.get_attestation(ids[0]);
  ASSERT_TRUE(got);
  EXPECT_EQ("42", got->arq_amount_atomic);

  arq_etn::ReserveSummary r;
  r.as_of_height = 7;
  r.liability_atomic = "100";
  r.reserve_proof_blob = "proof";
  ASSERT_TRUE(store.save_reserve(r));
  const auto rr = store.latest_reserve();
  ASSERT_TRUE(rr);
  EXPECT_EQ(7u, rr->as_of_height);
  EXPECT_EQ("test-issuer", rr->issuer_id);
  std::filesystem::remove_all(dir);
}
