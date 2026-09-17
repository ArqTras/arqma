// Copyright (c) 2018 - 2026, The Arqma Network

#include "etn_types.h"

#include <cstdio>
#include <sstream>

namespace arq_etn {
namespace {
std::string json_escape(const std::string& s)
{
  std::string out;
  out.reserve(s.size() + 8);
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') {
      out.push_back('\\');
      out.push_back(static_cast<char>(c));
    } else if (c < 0x20) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\u%04x", c);
      out += buf;
    } else {
      out.push_back(static_cast<char>(c));
    }
  }
  return out;
}

std::string json_string_field(const std::string& json, const std::string& key)
{
  const std::string needle = "\"" + key + "\":\"";
  const auto start = json.find(needle);
  if (start == std::string::npos)
    return {};
  auto i = start + needle.size();
  std::string out;
  while (i < json.size() && json[i] != '"') {
    if (json[i] == '\\' && i + 1 < json.size()) {
      out.push_back(json[i + 1]);
      i += 2;
      continue;
    }
    out.push_back(json[i++]);
  }
  return out;
}

std::uint64_t json_u64_field(const std::string& json, const std::string& key)
{
  const std::string needle = "\"" + key + "\":";
  const auto start = json.find(needle);
  if (start == std::string::npos)
    return 0;
  auto i = start + needle.size();
  while (i < json.size() && (json[i] == ' ' || json[i] == '\t'))
    ++i;
  std::uint64_t v = 0;
  while (i < json.size() && json[i] >= '0' && json[i] <= '9') {
    v = v * 10 + static_cast<std::uint64_t>(json[i] - '0');
    ++i;
  }
  return v;
}
} // namespace

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

std::string direction_to_string(WrapDirection d) noexcept
{
  return d == WrapDirection::Redeem ? "redeem" : "mint";
}

WrapDirection direction_from_string(const std::string& s) noexcept
{
  return s == "redeem" ? WrapDirection::Redeem : WrapDirection::Mint;
}

std::string reserve_to_json(const ReserveSummary& r)
{
  std::ostringstream o;
  o << "{\"schema\":\"arqma-etn-por-v1\""
    << ",\"as_of_height\":" << r.as_of_height
    << ",\"liability_atomic\":\"" << json_escape(r.liability_atomic) << "\""
    << ",\"wallet_balance_atomic\":\"" << json_escape(r.wallet_balance_atomic) << "\""
    << ",\"reserve_proof\":\"" << json_escape(r.reserve_proof_blob) << "\""
    << ",\"issuer_id\":\"" << json_escape(r.issuer_id) << "\"}";
  return o.str();
}

bool reserve_from_json(const std::string& json, ReserveSummary& out)
{
  out = {};
  out.as_of_height = json_u64_field(json, "as_of_height");
  out.liability_atomic = json_string_field(json, "liability_atomic");
  out.wallet_balance_atomic = json_string_field(json, "wallet_balance_atomic");
  out.reserve_proof_blob = json_string_field(json, "reserve_proof");
  if (out.reserve_proof_blob.empty())
    out.reserve_proof_blob = json_string_field(json, "reserve_proof_blob");
  out.issuer_id = json_string_field(json, "issuer_id");
  return !out.issuer_id.empty() || !out.reserve_proof_blob.empty();
}

std::string attestation_to_json(const BurnMintAttestation& a)
{
  std::ostringstream o;
  o << "{\"id\":\"" << json_escape(a.id) << "\""
    << ",\"direction\":\"" << direction_to_string(a.direction) << "\""
    << ",\"arq_txid\":\"" << json_escape(a.arq_txid_hex) << "\""
    << ",\"arq_amount_atomic\":\"" << json_escape(a.arq_amount_atomic) << "\""
    << ",\"eth_txid\":\"" << json_escape(a.eth_txid_hex) << "\""
    << ",\"warq_amount_atomic\":\"" << json_escape(a.warq_amount_atomic) << "\""
    << ",\"eth_chain_id\":" << a.eth_chain_id
    << ",\"arq_height\":" << a.arq_height
    << ",\"issuer_pubkey\":\"" << json_escape(a.issuer_pubkey_hex) << "\""
    << ",\"signature\":\"" << json_escape(a.signature_hex) << "\"}";
  return o.str();
}

bool attestation_from_json(const std::string& json, BurnMintAttestation& out)
{
  out = {};
  out.id = json_string_field(json, "id");
  out.direction = direction_from_string(json_string_field(json, "direction"));
  out.arq_txid_hex = json_string_field(json, "arq_txid");
  if (out.arq_txid_hex.empty())
    out.arq_txid_hex = json_string_field(json, "arq_txid_hex");
  out.arq_amount_atomic = json_string_field(json, "arq_amount_atomic");
  out.eth_txid_hex = json_string_field(json, "eth_txid");
  if (out.eth_txid_hex.empty())
    out.eth_txid_hex = json_string_field(json, "eth_txid_hex");
  out.warq_amount_atomic = json_string_field(json, "warq_amount_atomic");
  out.eth_chain_id = json_u64_field(json, "eth_chain_id");
  out.arq_height = json_u64_field(json, "arq_height");
  out.issuer_pubkey_hex = json_string_field(json, "issuer_pubkey");
  if (out.issuer_pubkey_hex.empty())
    out.issuer_pubkey_hex = json_string_field(json, "issuer_pubkey_hex");
  out.signature_hex = json_string_field(json, "signature");
  if (out.signature_hex.empty())
    out.signature_hex = json_string_field(json, "signature_hex");
  return attestation_fields_present(out);
}

} // namespace arq_etn
