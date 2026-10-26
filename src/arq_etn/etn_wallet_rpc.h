// Copyright (c) 2018 - 2026, The Arqma Network

#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace arq_etn {

struct WalletRpcConfig
{
  std::string url; // http://host:port[/json_rpc]
};

struct ReserveProofResult
{
  bool ok = false;
  std::string signature;
  std::string error;
  std::uint64_t wallet_height = 0;
  std::string balance_atomic; // unlocked balance if available
};

/// Call arqma-wallet-rpc `get_reserve_proof` (+ optional `get_height` / `get_balance`).
ReserveProofResult fetch_reserve_proof_from_wallet_rpc(const WalletRpcConfig& cfg,
                                                       const std::string& message = "arqma-etn-por");

} // namespace arq_etn
