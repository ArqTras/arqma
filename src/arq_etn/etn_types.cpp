// Copyright (c) 2018 - 2026, The Arqma Network

#include "etn_types.h"

namespace arq_etn {

bool attestation_fields_present(const BurnMintAttestation& a) noexcept
{
  if (a.arq_txid_hex.empty() || a.arq_amount_atomic.empty())
    return false;
  if (a.eth_txid_hex.empty() || a.warq_amount_atomic.empty())
    return false;
  if (a.issuer_pubkey_hex.empty() || a.signature_hex.empty())
    return false;
  if (a.eth_chain_id == 0)
    return false;
  return true;
}

} // namespace arq_etn
