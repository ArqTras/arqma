// Copyright (c) 2018 - 2026, The Arqma Network

#pragma once

#include "etn_types.h"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace arq_etn {

/// Simple file-backed store under data_dir/{reserve.json,attestations/<id>.json}.
class EtNStore
{
public:
  void set_data_dir(std::string path);
  const std::string& data_dir() const noexcept { return data_dir_; }

  void set_issuer_id(std::string id);
  const std::string& issuer_id() const noexcept { return issuer_id_; }

  void load();
  bool save_reserve(const ReserveSummary& r);
  std::optional<ReserveSummary> latest_reserve() const;

  bool put_attestation(BurnMintAttestation a);
  std::optional<BurnMintAttestation> get_attestation(const std::string& id) const;
  std::vector<std::string> list_attestation_ids() const;
  std::size_t attestation_count() const;

private:
  mutable std::mutex mu_;
  std::string data_dir_;
  std::string issuer_id_{"arqma-etn-issuer"};
  std::optional<ReserveSummary> reserve_;
  std::unordered_map<std::string, BurnMintAttestation> attestations_;
};

} // namespace arq_etn
