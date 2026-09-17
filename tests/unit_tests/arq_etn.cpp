// Copyright (c) 2018 - 2026, The Arqma Network

#include "gtest/gtest.h"

#include "arq_etn/etn_types.h"

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
