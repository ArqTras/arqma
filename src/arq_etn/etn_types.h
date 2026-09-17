// Copyright (c) 2018 - 2026, The Arqma Network
//
// Sketch types for ETN custody (model A) + burn↔mint wrap attestations (model C).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace arq_etn {

enum class WrapDirection : std::uint8_t
{
  Mint = 1,
  Redeem = 2
};

struct ReserveSummary
{
  std::uint64_t as_of_height = 0;
  std::string liability_atomic;
  std::string reserve_proof_blob;
  std::string issuer_id;
};

struct BurnMintAttestation
{
  WrapDirection direction = WrapDirection::Mint;
  std::string id;
  std::string arq_txid_hex;
  std::string arq_amount_atomic;
  std::string eth_txid_hex;
  std::string warq_amount_atomic;
  std::uint64_t eth_chain_id = 0;
  std::uint64_t arq_height = 0;
  std::string issuer_pubkey_hex;
  std::string signature_hex;
};

bool attestation_fields_present(const BurnMintAttestation& a) noexcept;
std::string direction_to_string(WrapDirection d) noexcept;
WrapDirection direction_from_string(const std::string& s) noexcept;

std::string reserve_to_json(const ReserveSummary& r);
bool reserve_from_json(const std::string& json, ReserveSummary& out);
std::string attestation_to_json(const BurnMintAttestation& a);
bool attestation_from_json(const std::string& json, BurnMintAttestation& out);

} // namespace arq_etn
