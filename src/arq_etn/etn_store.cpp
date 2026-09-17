// Copyright (c) 2018 - 2026, The Arqma Network

#include "etn_store.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace arq_etn {
namespace {
std::string make_id(const BurnMintAttestation& a)
{
  if (!a.id.empty())
    return a.id;
  return a.arq_txid_hex + "-" + direction_to_string(a.direction);
}
} // namespace

void EtNStore::set_data_dir(std::string path)
{
  std::lock_guard<std::mutex> lock{mu_};
  data_dir_ = std::move(path);
}

void EtNStore::set_issuer_id(std::string id)
{
  std::lock_guard<std::mutex> lock{mu_};
  if (!id.empty())
    issuer_id_ = std::move(id);
}

void EtNStore::load()
{
  std::lock_guard<std::mutex> lock{mu_};
  attestations_.clear();
  reserve_.reset();
  if (data_dir_.empty())
    return;
  std::error_code ec;
  std::filesystem::create_directories(data_dir_, ec);
  std::filesystem::create_directories(std::filesystem::path{data_dir_} / "attestations", ec);
  {
    std::ifstream in{std::filesystem::path{data_dir_} / "reserve.json"};
    if (in) {
      std::string body((std::istreambuf_iterator<char>(in)), {});
      ReserveSummary r;
      if (reserve_from_json(body, r))
        reserve_ = std::move(r);
    }
  }
  for (const auto& ent : std::filesystem::directory_iterator{std::filesystem::path{data_dir_} / "attestations", ec}) {
    if (!ent.is_regular_file())
      continue;
    std::ifstream in{ent.path()};
    if (!in)
      continue;
    std::string body((std::istreambuf_iterator<char>(in)), {});
    BurnMintAttestation a;
    if (!attestation_from_json(body, a))
      continue;
    if (a.id.empty())
      a.id = ent.path().stem().string();
    attestations_[a.id] = std::move(a);
  }
}

bool EtNStore::save_reserve(const ReserveSummary& r)
{
  std::lock_guard<std::mutex> lock{mu_};
  ReserveSummary copy = r;
  if (copy.issuer_id.empty())
    copy.issuer_id = issuer_id_;
  reserve_ = copy;
  if (data_dir_.empty())
    return true;
  std::error_code ec;
  std::filesystem::create_directories(data_dir_, ec);
  std::ofstream out{std::filesystem::path{data_dir_} / "reserve.json", std::ios::trunc};
  if (!out)
    return false;
  out << reserve_to_json(copy);
  return static_cast<bool>(out);
}

std::optional<ReserveSummary> EtNStore::latest_reserve() const
{
  std::lock_guard<std::mutex> lock{mu_};
  return reserve_;
}

bool EtNStore::put_attestation(BurnMintAttestation a)
{
  if (!attestation_fields_present(a))
    return false;
  a.id = make_id(a);
  std::lock_guard<std::mutex> lock{mu_};
  if (data_dir_.empty()) {
    attestations_[a.id] = a;
    return true;
  }
  std::error_code ec;
  const auto dir = std::filesystem::path{data_dir_} / "attestations";
  std::filesystem::create_directories(dir, ec);
  std::ofstream out{dir / (a.id + ".json"), std::ios::trunc};
  if (!out)
    return false;
  out << attestation_to_json(a);
  if (!out)
    return false;
  attestations_[a.id] = std::move(a);
  return true;
}

std::optional<BurnMintAttestation> EtNStore::get_attestation(const std::string& id) const
{
  std::lock_guard<std::mutex> lock{mu_};
  const auto it = attestations_.find(id);
  if (it == attestations_.end())
    return std::nullopt;
  return it->second;
}

std::vector<std::string> EtNStore::list_attestation_ids() const
{
  std::lock_guard<std::mutex> lock{mu_};
  std::vector<std::string> ids;
  ids.reserve(attestations_.size());
  for (const auto& kv : attestations_)
    ids.push_back(kv.first);
  return ids;
}

std::size_t EtNStore::attestation_count() const
{
  std::lock_guard<std::mutex> lock{mu_};
  return attestations_.size();
}

} // namespace arq_etn
