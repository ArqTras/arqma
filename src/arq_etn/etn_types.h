// Copyright (c) 2018 - 2026, The Arqma Network
//
// Sketch types for ETN custody (model A) + burn↔mint wrap attestations (model C).
// Not consensus. Optional build: -DBUILD_ARQ_ETN=ON

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace arq_etn {

enum class WrapDirection : std::uint8_t
{
  Mint = 1,   // ARQ burned → wARQ minted
  Redeem = 2  // wARQ burned → ARQ paid from custody
};

/// Aggregated reserve snapshot for an issuer (model A).
struct ReserveSummary
{
  std::uint64_t as_of_height = 0;
  std::string liability_atomic;     // decimal string of outstanding note units
  std::string reserve_proof_blob;   // wallet2 get_reserve_proof payload
  std::string issuer_id;
};

/// Links an Arqma HF19 burn (or custody payout) to an external wrap tx (model C).
struct BurnMintAttestation
{
  WrapDirection direction = WrapDirection::Mint;
  std::string arq_txid_hex;
  std::string arq_amount_atomic;
  std::string eth_txid_hex;
  std::string warq_amount_atomic;
  std::uint64_t eth_chain_id = 0;
  std::uint64_t arq_height = 0;
  std::string issuer_pubkey_hex;
  std::string signature_hex;
};

/// Minimal validation used by unit tests / companion (no wallet I/O here).
bool attestation_fields_present(const BurnMintAttestation& a) noexcept;

} // namespace arq_etn
