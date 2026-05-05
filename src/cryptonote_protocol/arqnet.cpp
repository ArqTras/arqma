#include "arqnet.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/service_node_voting.h"
#include "cryptonote_core/service_node_rules.h"
#include "cryptonote_core/tx_pool.h"
#include "arqnet/sn_network.h"
#include "arqnet/pulse_wire.h"
#include "arqnet/conn_matrix.h"
#include "common/arqma_pulse_fork.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/verification_context.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <unordered_map>

#undef ARQMA_DEFAULT_LOG_CATEGORY
#define ARQMA_DEFAULT_LOG_CATEGORY "arqnet"

namespace arqnet {

namespace {

using namespace service_nodes;
using namespace std::string_literals;
using namespace std::chrono_literals;

struct crypto_hash_hasher {
  size_t operator()(crypto::hash const &h) const noexcept
  {
    uint64_t a, b;
    static_assert(sizeof(h.data) >= 16, "");
    std::memcpy(&a, h.data, sizeof(a));
    std::memcpy(&b, h.data + 8, sizeof(b));
    return static_cast<size_t>(a ^ (b << 1U));
  }
};

struct pulse_vote_accumulator {
  std::mutex mu;
  struct rec {
    uint64_t block_height = 0;
    std::unordered_map<uint16_t, cryptonote::pulse_validator_signature_entry> rows;
    std::chrono::steady_clock::time_point touch{};
  };
  std::unordered_map<crypto::hash, rec, crypto_hash_hasher> by_bh;
  static constexpr size_t max_blocks = 64;

  void prune_unlocked()
  {
    while (by_bh.size() > max_blocks)
    {
      auto oldest = by_bh.begin();
      for (auto it = by_bh.begin(); it != by_bh.end(); ++it)
      {
        if (it->second.touch < oldest->second.touch)
          oldest = it;
      }
      by_bh.erase(oldest);
    }
  }

  void add(crypto::hash const &bh, uint64_t block_h, uint16_t vi, cryptonote::pulse_validator_signature_entry const &e)
  {
    std::lock_guard<std::mutex> lk(mu);
    rec &r = by_bh[bh];
    r.block_height = block_h;
    r.touch = std::chrono::steady_clock::now();
    r.rows[vi] = e;
    prune_unlocked();
  }

  bool copy(crypto::hash const &bh, uint64_t *height_out, std::vector<cryptonote::pulse_validator_signature_entry> *out)
  {
    if (!height_out || !out)
      return false;
    std::lock_guard<std::mutex> lk(mu);
    auto it = by_bh.find(bh);
    if (it == by_bh.end())
      return false;
    *height_out = it->second.block_height;
    out->clear();
    out->reserve(it->second.rows.size());
    for (auto const &kv : it->second.rows)
      out->push_back(kv.second);
    std::sort(out->begin(), out->end(), [](auto const &a, auto const &b) { return a.voter_index < b.voter_index; });
    return true;
  }

  void clear(crypto::hash const &bh)
  {
    std::lock_guard<std::mutex> lk(mu);
    by_bh.erase(bh);
  }
};

pulse_vote_accumulator g_pulse_vote_accum;

struct pulse_seen_recent {
  std::mutex mu;
  std::unordered_map<crypto::hash, std::chrono::steady_clock::time_point, crypto_hash_hasher> seen;
  static constexpr std::chrono::seconds ttl{45};
  static constexpr size_t max_entries = 4096;

  void prune_unlocked(std::chrono::steady_clock::time_point const now)
  {
    for (auto it = seen.begin(); it != seen.end();)
    {
      if (now - it->second > ttl)
        it = seen.erase(it);
      else
        ++it;
    }
    while (seen.size() > max_entries)
    {
      auto oldest = seen.begin();
      for (auto it = seen.begin(); it != seen.end(); ++it)
      {
        if (it->second < oldest->second)
          oldest = it;
      }
      seen.erase(oldest);
    }
  }

  /** @return true if \p fp is still in the success-only window (duplicate inbound). */
  bool is_recent(crypto::hash const &fp)
  {
    auto const now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lk(mu);
    prune_unlocked(now);
    return seen.find(fp) != seen.end();
  }

  /** Record a successfully handled inbound fingerprint (proposal \p bh or vote pack hash). */
  void mark_seen(crypto::hash const &fp)
  {
    auto const now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lk(mu);
    seen.insert_or_assign(fp, now);
    prune_unlocked(now);
  }
};

pulse_seen_recent g_pulse_seen_inbound;

struct SNNWrapper {
  SNNetwork snn;
  cryptonote::core &core;

  template <typename... Args>
  SNNWrapper(cryptonote::core &core, Args &&...args)
    : snn{std::forward<Args>(args)...}, core{core} {}

  static SNNWrapper &from(void* obj)
  {
    assert(obj);
    return *reinterpret_cast<SNNWrapper*>(obj);
  }
};

template <typename T>
std::string get_data_as_string(const T &key)
{
  static_assert(std::is_trivial<T>(), "cannot safely copy non-trivial class to string");
  return {reinterpret_cast<const char *>(&key), sizeof(key)};
}

crypto::x25519_public_key x25519_from_string(std::string_view pubkey)
{
  crypto::x25519_public_key x25519_pub = crypto::x25519_public_key::null();
  if (pubkey.size() == sizeof(crypto::x25519_public_key))
    std::memcpy(x25519_pub.data, pubkey.data(), pubkey.size());
  return x25519_pub;
}

std::string get_connect_string(const service_node_list &sn_list, const crypto::x25519_public_key &x25519_pub)
{
  if (!x25519_pub)
  {
    MDEBUG("no connection available: pubkey is epmty");
    return "";
  }
  auto pubkey = sn_list.get_pubkey_from_x25519(x25519_pub);
  if (!pubkey)
  {
    MDEBUG("no connection available: could not find primary pubkey from x25519 pubkey " << x25519_pub);
    return "";
  }
  bool found = false;
  uint32_t ip = 0;
  uint16_t port = 0;
  sn_list.for_each_service_node_info_and_proof(&pubkey, &pubkey + 1, [&](auto&, auto&, auto& proof)
  {
    found = true;
    ip = proof.public_ip;
    port = proof.arqnet_port;
  });
  if (!found)
  {
    MDEBUG("no connection available: primary pubkey " << pubkey << " is not registered");
    return "";
  }
  if (!(ip && port))
  {
    MDEBUG("no connection available: service node " << pubkey << " has no associated ip and/or port");
    return "";
  }
  return "tcp://" + epee::string_tools::get_ip_string_from_int32(ip) + ":" + std::to_string(port);
}

constexpr el::Level easylogging_level(LogLevel level)
{
  switch (level)
  {
    case LogLevel::fatal: return el::Level::Fatal;
    case LogLevel::error: return el::Level::Error;
    case LogLevel::warn: return el::Level::Warning;
    case LogLevel::info: return el::Level::Info;
    case LogLevel::debug: return el::Level::Debug;
    case LogLevel::trace: return el::Level::Trace;
  };
  return el::Level::Unknown;
};

bool snn_want_log(LogLevel level)
{
  return ELPP->vRegistry()->allowed(easylogging_level(level), ARQMA_DEFAULT_LOG_CATEGORY);
}

void snn_write_log(LogLevel level, const char *file, int line, std::string msg)
{
  el::base::Writer(easylogging_level(level), el::Color::Default, file, line, ELPP_FUNC, el::base::DispatchAction::NormalLog).construct(ARQMA_DEFAULT_LOG_CATEGORY) << msg;
}

void *new_snnwrapper(cryptonote::core &core, const std::string &bind)
{
  auto keys = core.get_service_node_keys();
  auto peer_lookup = [&sn_list = core.get_service_node_list()](const std::string &x25519_pub)
  {
    return get_connect_string(sn_list, x25519_from_string(x25519_pub));
  };
  auto allow = [&sn_list = core.get_service_node_list()](const std::string &ip, const std::string &x25519_pubkey_str)
  {
    auto x25519_pubkey = x25519_from_string(x25519_pubkey_str);
    auto pubkey = sn_list.get_pubkey_from_x25519(x25519_pubkey);
    if (pubkey)
    {
      MINFO("Accepting incoming SN connection authentication from ip/x25519/pubkey: " << ip << "/" << x25519_pubkey << "/" << pubkey);
      return SNNetwork::allow::service_node;
    }

    return SNNetwork::allow::client;
  };
  SNNWrapper *obj;
  if (!keys)
  {
    MINFO("Starting remote-only arqnet instance");
    obj = new SNNWrapper(core, peer_lookup, allow, snn_want_log, snn_write_log);
  }
  else
  {
    MINFO("Starting arqnet listener on " << bind << " with x25519 pubkey " << keys->pub_x25519);
    obj = new SNNWrapper(core, get_data_as_string(keys->pub_x25519), get_data_as_string(keys->key_x25519.data),
                         std::vector<std::string>{{bind}}, peer_lookup, allow, snn_want_log, snn_write_log);
  }

  obj->snn.data = obj;

  return obj;
}

void delete_snnwrapper(void *&obj)
{
  auto *snn = reinterpret_cast<SNNWrapper *>(obj);
  MINFO("Shutting down arqnet listener");
  delete snn;
  obj = nullptr;
}

template <typename E>
#ifdef __GNUG__
[[gnu::warn_unused_result]]
#endif
E get_enum(const bt_dict &d, const std::string &key)
{
  E result = static_cast<E>(get_int<std::underlying_type_t<E>>(d.at(key)));
  if (result < E::_count)
    return result;
  throw std::invalid_argument("invalid enum value for field " + key);
}

class peer_info {
public:
  using exclude_set = std::unordered_set<crypto::public_key>;

  std::unordered_map<crypto::public_key, std::pair<crypto::x25519_public_key, std::string>> remotes;
  std::unordered_map<std::string, std::string> peers;
  int strong_peers;
  std::vector<int> my_position;
  int my_position_count;

  peer_info(SNNWrapper &snw, quorum_type q_type, std::shared_ptr<const quorum> &quorum, bool opportunistic = true, exclude_set exclude = {})
    : peer_info(snw, q_type, &quorum, &quorum + 1, opportunistic, std::move(exclude))
  {}

  template <typename QuorumIt>
  peer_info(SNNWrapper &snw, quorum_type q_type, QuorumIt qbegin, QuorumIt qend, bool opportunistic = true, std::unordered_set<crypto::public_key> exclude = {})
    : snn{snw.snn}
  {
    auto keys = snw.core.get_service_node_keys();
    assert(keys);
    const auto &my_pubkey = keys->pub;
    exclude.insert(my_pubkey);

    my_position_count = 0;
    std::unordered_set<crypto::public_key> need_remotes;
    for (auto qit = qbegin; qit != qend; ++qit)
    {
      auto &v = (*qit)->validators;
      int my_pos = -1;
      for (size_t i = 0; i < v.size(); i++)
      {
        if (v[i] == my_pubkey)
          my_pos = static_cast<int>(i);
        else if (!exclude.count(v[i]))
          need_remotes.insert(v[i]);
      }
      my_position.push_back(my_pos);
      if (my_pos >= 0)
        my_position_count++;
    }

    snw.core.get_service_node_list().for_each_service_node_info_and_proof(need_remotes.begin(), need_remotes.end(),
      [this](const auto &pubkey, const auto &info, const auto &proof)
      {
        if (info.is_active() && proof.pubkey_x25519 && proof.arqnet_port && proof.public_ip)
          remotes.emplace(pubkey, std::make_pair(proof.pubkey_x25519, "tcp://" + epee::string_tools::get_ip_string_from_int32(proof.public_ip) + ":" + std::to_string(proof.arqnet_port)));
      });
    compute_peers(qbegin, qend, opportunistic);
  }

  template <typename... T>
  void relay_to_peers(const std::string &cmd, const T &...data)
  {
    relay_to_peers_impl(cmd, std::array<send_option::serialized, sizeof...(T)>{send_option::serialized{data}...}, std::make_index_sequence<sizeof...(T)>{});
  }

private:
  SNNetwork &snn;

  bool add_peer(const crypto::public_key &pubkey, bool strong = true)
  {
    auto it = remotes.find(pubkey);
    if (it != remotes.end())
    {
      std::string remote_addr = strong ? it->second.second : ""s;
      auto ins = peers.emplace(get_data_as_string(it->second.first), std::move(remote_addr));
      if (strong && !ins.second && ins.first->second.empty())
      {
        ins.first->second = it->second.second;
        strong_peers++;
        return true;
      }
      if (strong && ins.second)
        strong_peers++;

      return ins.second;
    }
    return false;
  }

  template <typename QuorumIt>
  void compute_peers(QuorumIt qbegin, QuorumIt qend, bool opportunistic)
  {
    strong_peers = 0;

    size_t i = 0;
    for (QuorumIt qit = qbegin; qit != qend; ++i, ++qit)
    {
      if (my_position[i] < 0)
      {
        MTRACE("Not in quorum " << (i == 0 ? "Q" : "Q'"));
        continue;
      }

      auto &validators = (*qit)->validators;

      for (int j : quorum_outgoing_conns(my_position[i], validators.size()))
      {
        if (add_peer(validators[j]))
          MTRACE("Relaying within quorum " << (i == 0 ? "Q" : "Q'") << " to service node " << validators[j]);
      }

      for (int j : quorum_incoming_conns(my_position[i], validators.size()))
      {
        if (add_peer(validators[j], false))
          MTRACE("Optional opportunistic relay with quorum " << (i == 0 ? "Q" : "Q'") << " to service node " << validators[j]);
      }

      QuorumIt qnext = std::next(qit);
      if (qnext != qend && my_position[i + 1] < 0)
      {
        auto &next_validators = (*qnext)->validators;
        int half = std::min<int>(validators.size(), next_validators.size()) / 2;
        if (my_position[i] >= half && my_position[i] < half*2)
        {
          if (add_peer(validators[my_position[i] - half]))
            MTRACE("Inter-quorum relay from Q to Q' service node " << next_validators[half + my_position[i]]);
        }
        else
        {
          MTRACE("Not a Q -> Q' inter-quorum relay (Q position is " << my_position[i] << ")");
        }
      }

      if (qit != qbegin && my_position[i - 1] < 0)
      {
        auto &prev_validators = (*std::prev(qit))->validators;
        int half = std::min<int>(validators.size(), prev_validators.size()) / 2;
        if (my_position[i] < half)
        {
          if (add_peer(prev_validators[half + my_position[i]]))
            MTRACE("Inter-quorum relay from Q' to Q service node " << prev_validators[my_position[i] - half]);
        }
        else
        {
          MTRACE("Not a Q' -> Q inter-quorum relay (Q' position is " << my_position[i] << ")");
        }
      }
    }
  }

  template<size_t N, size_t... I>
  void relay_to_peers_impl(const std::string &cmd, std::array<send_option::serialized, N> relay_data, std::index_sequence<I...>)
  {
    for (auto &peer : peers)
    {
      MTRACE("Relaying " << cmd << " to peer " << as_hex(peer.first) << (peer.second.empty() ? " (if connected)"s : " @ " + peer.second));
      if (peer.second.empty())
        snn.send(peer.first, cmd, relay_data[I]..., send_option::optional{});
      else
        snn.send(peer.first, cmd, relay_data[I]..., send_option::hint{peer.second});
    }
  }

};

bt_dict serialize_vote(const quorum_vote_t &vote)
{
  bt_dict result{
    {"v", vote.version},
    {"t", static_cast<uint8_t>(vote.type)},
    {"h", vote.block_height},
    {"g", static_cast<uint8_t>(vote.group)},
    {"i", vote.index_in_group},
    {"s", get_data_as_string(vote.signature)},
  };
  if (vote.type == quorum_type::checkpointing)
    result["bh"] = std::string{vote.checkpoint.block_hash.data, sizeof(crypto::hash)};
  else
  {
    result["wi"] = vote.state_change.worker_index;
    result["sc"] = static_cast<std::underlying_type_t<new_state>>(vote.state_change.state);
  }
  return result;
}

std::vector<std::shared_ptr<const quorum>> checkpoint_try_quorums(service_node_list const &snl, uint64_t quorum_height)
{
  std::vector<std::shared_ptr<const quorum>> alt_quorums;
  std::shared_ptr<const quorum> const main_q =
      snl.get_quorum(quorum_type::checkpointing, quorum_height, true, &alt_quorums);

  std::vector<std::shared_ptr<const quorum>> try_quorums;
  if (main_q)
    try_quorums.push_back(main_q);
  for (std::shared_ptr<const quorum> const &aq : alt_quorums)
  {
    if (aq && aq != main_q)
      try_quorums.push_back(aq);
  }
  return try_quorums;
}

void relay_pulse_quorum_peers(SNNWrapper &snw, uint64_t quorum_height, std::string const &cmd, bt_dict const &payload)
{
  if (!snw.core.get_service_node_keys())
    return;
  std::vector<std::shared_ptr<const quorum>> const try_quorums =
      checkpoint_try_quorums(snw.core.get_service_node_list(), quorum_height);
  if (try_quorums.empty())
    return;
  peer_info pinfo(snw, quorum_type::checkpointing, try_quorums.begin(), try_quorums.end());
  if (!pinfo.my_position_count || pinfo.peers.empty())
    return;
  MDEBUG("pulse relay " << cmd << " -> " << pinfo.peers.size() << " quorum peers");
  pinfo.relay_to_peers(cmd, payload);
}

quorum_vote_t deserialize_vote(const bt_value &v)
{
  const auto &d = boost::get<bt_dict>(v);
  quorum_vote_t vote;
  vote.version = get_int<uint8_t>(d.at("v"));
  vote.type = get_enum<quorum_type>(d, "t");
  vote.block_height = get_int<uint64_t>(d.at("h"));
  vote.group = get_enum<quorum_group>(d, "g");
  if (vote.group == quorum_group::invalid) throw std::invalid_argument("invalid vote group");
  vote.index_in_group = get_int<uint16_t>(d.at("i"));
  auto &sig = boost::get<std::string>(d.at("s"));
  if (sig.size() != sizeof(vote.signature)) throw std::invalid_argument("invalid vote signature size");
  std::memcpy(&vote.signature, sig.data(), sizeof(vote.signature));
  if (vote.type == quorum_type::checkpointing)
  {
    auto &bh = boost::get<std::string>(d.at("bh"));
    if (bh.size() != sizeof(vote.checkpoint.block_hash.data)) throw std::invalid_argument("invalid vote checkpoint block hash");
    std::memcpy(vote.checkpoint.block_hash.data, bh.data(), sizeof(vote.checkpoint.block_hash.data));
  }
  else
  {
    vote.state_change.worker_index = get_int<uint16_t>(d.at("wi"));
    vote.state_change.state = get_enum<new_state>(d, "sc");
  }

  return vote;
}

void relay_obligation_votes(void *obj, const std::vector<service_nodes::quorum_vote_t> &votes)
{
  auto &snw = SNNWrapper::from(obj);
  auto my_keys_ptr = snw.core.get_service_node_keys();
  assert(my_keys_ptr);
  const auto &my_keys = *my_keys_ptr;

  MDEBUG("Starting relay of " << votes.size() << " votes");
  std::vector<service_nodes::quorum_vote_t> relayed_votes;
  relayed_votes.reserve(votes.size());

  // Group votes by quorum to avoid recreating peer_info for the same quorum
  std::map<std::pair<quorum_type, uint64_t>, std::vector<const service_nodes::quorum_vote_t*>> votes_by_quorum;
  for (const auto& vote : votes)
  {
    if (vote.type != quorum_type::obligations)
    {
      MERROR("Internal logic error: arqnet asked to relay a " << vote.type << " vote, but should only be called with obligations votes");
      continue;
    }
    votes_by_quorum[{vote.type, vote.block_height}].push_back(&vote);
  }

  MDEBUG("Grouped " << votes.size() << " votes info " << votes_by_quorum.size() << " quorum groups");

  size_t vote_index = 0;
  for (auto &[quorum_key, quorum_votes] : votes_by_quorum)
  {
    auto [q_type, block_height] = quorum_key;
    vote_index += quorum_votes.size();
    if (vote_index % 50 == 0 || vote_index == votes.size())
      MDEBUG("Processing vote group " << vote_index - quorum_votes.size() + 1 << "-" << vote_index << " of " << votes.size() << " (quorum at height " << block_height << ")");

    auto quorum = snw.core.get_service_node_list().get_quorum(q_type, block_height);
    if (!quorum)
    {
      MWARNING("Unable to relay " << quorum_votes.size() << " votes: no " << q_type << " quorum available for height " << block_height);
      continue;
    }

    auto &quorum_voters = quorum->validators;
    if (quorum_voters.size() < service_nodes::min_votes_for_quorum_type(q_type))
    {
      MWARNING("Invalid vote relay: " << q_type << " quorum @ height " << block_height << " does not have enough validators ("
               << quorum_voters.size() << ") to reach the minimum required votes ("
               << service_nodes::min_votes_for_quorum_type(q_type) << ")");
    }

    MTRACE("Creating peer_info for quorum at height " << block_height << " with " << quorum_votes.size() << " votes");
    peer_info pinfo{snw, q_type, quorum};
    if (!pinfo.my_position_count)
    {
      MWARNING("Invalid vote relay: quorum at height " << block_height << " does not include this service node");
      continue;
    }

    MTRACE("Relaying " << quorum_votes.size() << " votes to " << pinfo.peers.size() << " peers");
    for (const auto *vote_ptr : quorum_votes)
    {
      pinfo.relay_to_peers("vote_ob", serialize_vote(*vote_ptr));
      relayed_votes.push_back(*vote_ptr);
    }
  }
  MDEBUG("Relayed " << relayed_votes.size() << " votes");
  snw.core.set_service_node_votes_relayed(relayed_votes);
}

void handle_obligation_vote(SNNetwork::message &m, void *self)
{
  auto &snw = SNNWrapper::from(self);

  MDEBUG("Received a relayed obligation vote from " << as_hex(m.pubkey));

  if (m.data.size() != 1)
  {
    MINFO("Ignoring vote: expected 1 data part, not " << m.data.size());
    return;
  }

  try
  {
    std::vector<quorum_vote_t> vvote;
    vvote.push_back(deserialize_vote(m.data[0]));
    auto& vote = vvote.back();

    if (vote.type != quorum_type::obligations)
    {
      MWARNING("Received invalid vote via arqnet. Ignoring");
      return;
    }

    if (vote.block_height > snw.core.get_current_blockchain_height())
    {
      MDEBUG("Ignoring vote: block height " << vote.block_height << " is too high");
      return;
    }

    cryptonote::vote_verification_context vvc{};
    snw.core.add_service_node_vote(vote, vvc);
    if (vvc.m_verification_failed)
    {
      MWARNING("Vote verification failed: ignoring vote");
      return;
    }

    if (vvc.m_added_to_pool)
      relay_obligation_votes(self, std::move(vvote));
  }
  catch (const std::exception &e)
  {
    MWARNING("Deserialization of vote from " << as_hex(m.pubkey) << " failed: " << e.what());
  }
}

template <typename I>
std::enable_if_t<std::is_integral<I>::value, I> get_or(bt_dict &d, const std::string &key, I fallback)
{
  auto it = d.find(key);
  if (it != d.end())
  {
    try { return get_int<I>(it->second); }
    catch (...) { }
  }
  return fallback;
}

template <typename I>
std::enable_if_t<std::is_integral<I>::value, I> get_or(bt_dict const &d, const std::string &key, I fallback)
{
  auto it = d.find(key);
  if (it != d.end())
  {
    try { return get_int<I>(it->second); }
    catch (...) { }
  }
  return fallback;
}

void handle_ping(SNNetwork::message &m, void *)
{
  uint64_t tag = 0;
  if (!m.data.empty())
  {
    auto &data = boost::get<bt_dict>(m.data[0]);
    tag = get_or<uint64_t>(data, "!", 0);
  }

  MINFO("Received ping request from " << (m.sn ? "SN" : "non-SN") << " " << as_hex(m.pubkey) << ", sending pong");
  m.reply("pong", bt_dict{{"!", tag}, {"sn", m.sn}});
}

void handle_pong(SNNetwork::message &m, void *)
{
  MINFO("Received pong from " << (m.sn ? "SN" : "non-SN") << " " << as_hex(m.pubkey));
}

void handle_pulse_cap(SNNetwork::message &m, void *self)
{
  if (!self)
    return;
  auto &snw = SNNWrapper::from(self);
  uint64_t tag = 0;
  if (!m.data.empty())
  {
    try
    {
      auto &d = boost::get<bt_dict>(m.data[0]);
      tag = get_or<uint64_t>(d, arqma::pulse_wire::KEY_TAG, 0);
    }
    catch (...) { }
  }

  cryptonote::Blockchain const &bc = snw.core.get_blockchain_storage();
  uint64_t const h = bc.get_current_blockchain_height();
  uint8_t const ideal_hf = bc.get_ideal_hard_fork_version(h);

  bt_dict out{
      {arqma::pulse_wire::KEY_TAG, tag},
      {arqma::pulse_wire::KEY_WIRE_VERSION, static_cast<uint64_t>(arqma::pulse_wire::WIRE_VERSION_V1)},
      {arqma::pulse_wire::KEY_FORK_ACTIVE, arqma::pulse_fork::FORK_ACTIVE ? 1ULL : 0ULL},
      {arqma::pulse_wire::KEY_CHAIN_HEIGHT, h},
      {arqma::pulse_wire::KEY_IDEAL_HF, static_cast<uint64_t>(ideal_hf)},
      {arqma::pulse_wire::KEY_PULSE_MAJOR_READY, ideal_hf >= cryptonote::network_version_20_pos ? 1ULL : 0ULL},
  };

  MDEBUG("pulse_cap from " << as_hex(m.pubkey) << ", height " << h << ", ideal_hf " << static_cast<unsigned>(ideal_hf));
  m.reply(arqma::pulse_wire::COMMAND_PULSE_CAP, out);
}

void handle_pulse_proposal(SNNetwork::message &m, void *self)
{
  if (!self)
    return;
  if (m.data.size() != 1)
  {
    MWARNING("pulse_proposal: expected 1 data part, got " << m.data.size());
    return;
  }
  auto &snw = SNNWrapper::from(self);

  try
  {
    bt_dict const &d = boost::get<bt_dict>(m.data[0]);
    std::string const &blk = boost::get<std::string>(d.at(arqma::pulse_wire::KEY_BLOCK_BLOB));
    if (blk.size() > arqma::pulse_wire::PULSE_PROPOSAL_MAX_BLOB_BYTES)
    {
      m.reply(arqma::pulse_wire::COMMAND_PULSE_PROPOSAL,
          bt_dict{{arqma::pulse_wire::KEY_OK, 0LL},
              {arqma::pulse_wire::KEY_ERR, std::string{"block blob too large"}}});
      return;
    }

    cryptonote::block b;
    crypto::hash bh;
    if (!cryptonote::parse_and_validate_block_from_blob(blk, b, bh))
    {
      m.reply(arqma::pulse_wire::COMMAND_PULSE_PROPOSAL,
          bt_dict{{arqma::pulse_wire::KEY_OK, 0LL},
              {arqma::pulse_wire::KEY_ERR, std::string{"parse failed"}}});
      return;
    }

    if (g_pulse_seen_inbound.is_recent(bh))
    {
      MDEBUG("pulse_proposal duplicate block " << bh << " (seen window), short-circuit");
      m.reply(arqma::pulse_wire::COMMAND_PULSE_PROPOSAL,
          bt_dict{{arqma::pulse_wire::KEY_OK, 1LL},
              {arqma::pulse_wire::KEY_BLOCK_HASH, std::string{reinterpret_cast<const char *>(bh.data), sizeof(bh.data)}}});
      return;
    }

    if (arqma::pulse_fork::FORK_ACTIVE && b.major_version >= cryptonote::network_version_20_pos)
    {
      cryptonote::block_verification_context bvc{};
      if (!snw.core.get_blockchain_storage().verify_pulse_fork_block_rules(b, bvc, "arqnet_pulse_proposal"))
      {
        m.reply(arqma::pulse_wire::COMMAND_PULSE_PROPOSAL,
            bt_dict{{arqma::pulse_wire::KEY_OK, 0LL},
                {arqma::pulse_wire::KEY_ERR, std::string{"pulse rules failed"}}});
        return;
      }
    }

    g_pulse_seen_inbound.mark_seen(bh);

    MDEBUG("pulse_proposal OK id " << bh << " from " << as_hex(m.pubkey));
    m.reply(arqma::pulse_wire::COMMAND_PULSE_PROPOSAL,
        bt_dict{{arqma::pulse_wire::KEY_OK, 1LL},
            {arqma::pulse_wire::KEY_BLOCK_HASH, std::string{reinterpret_cast<const char *>(bh.data), sizeof(bh.data)}}});

    uint64_t const block_h = cryptonote::get_block_height(b);
    if (block_h >= 1)
    {
      int64_t const rh_in = std::max(
          static_cast<int64_t>(0),
          get_or<int64_t>(d, arqma::pulse_wire::KEY_RELAY_HOPS, arqma::pulse_wire::DEFAULT_PULSE_RELAY_HOPS));
      if (rh_in > 0)
      {
        bt_dict fwd = d;
        fwd[arqma::pulse_wire::KEY_RELAY_HOPS] = static_cast<int64_t>(rh_in - 1);
        relay_pulse_quorum_peers(snw, block_h - 1, arqma::pulse_wire::COMMAND_PULSE_PROPOSAL, fwd);
      }
    }
  }
  catch (const std::exception &e)
  {
    m.reply(arqma::pulse_wire::COMMAND_PULSE_PROPOSAL,
        bt_dict{{arqma::pulse_wire::KEY_OK, 0LL}, {arqma::pulse_wire::KEY_ERR, std::string{e.what()}}});
  }
}

void handle_pulse_vote(SNNetwork::message &m, void *self)
{
  if (!self)
    return;
  if (m.data.size() != 1)
  {
    MWARNING("pulse_vote: expected 1 data part, got " << m.data.size());
    return;
  }
  auto &snw = SNNWrapper::from(self);

  try
  {
    bt_dict const &d = boost::get<bt_dict>(m.data[0]);
    std::string const &bh_s = boost::get<std::string>(d.at(arqma::pulse_wire::KEY_BLOCK_HASH));
    if (bh_s.size() != sizeof(crypto::hash))
      throw std::invalid_argument("bad block hash length");

    crypto::hash bh;
    std::memcpy(bh.data, bh_s.data(), sizeof(bh.data));

    uint64_t const block_height = get_int<uint64_t>(d.at(arqma::pulse_wire::KEY_CHAIN_HEIGHT));
    if (block_height < 1)
      throw std::invalid_argument("block height");

    uint16_t const voter_index = get_int<uint16_t>(d.at(arqma::pulse_wire::KEY_VOTER_INDEX));
    std::string const &sig_s = boost::get<std::string>(d.at(arqma::pulse_wire::KEY_SIGNATURE));
    if (sig_s.size() != sizeof(crypto::signature))
      throw std::invalid_argument("bad signature length");

    crypto::signature sig;
    std::memcpy(&sig, sig_s.data(), sizeof(sig));

    crypto::public_key const sender_pk =
        snw.core.get_service_node_list().get_pubkey_from_x25519(x25519_from_string(m.pubkey));
    if (sender_pk == crypto::null_pkey)
    {
      m.reply(arqma::pulse_wire::COMMAND_PULSE_VOTE,
          bt_dict{{arqma::pulse_wire::KEY_OK, 0LL},
              {arqma::pulse_wire::KEY_ERR, std::string{"unknown sender SN pubkey"}}});
      return;
    }

    std::vector<std::shared_ptr<const quorum>> const try_quorums =
        checkpoint_try_quorums(snw.core.get_service_node_list(), block_height - 1);
    if (try_quorums.empty())
    {
      m.reply(arqma::pulse_wire::COMMAND_PULSE_VOTE,
          bt_dict{{arqma::pulse_wire::KEY_OK, 0LL},
              {arqma::pulse_wire::KEY_ERR, std::string{"no checkpointing quorum"}}});
      return;
    }

    bool ok = false;
    for (std::shared_ptr<const quorum> const &qp : try_quorums)
    {
      if (!qp || voter_index >= qp->validators.size())
        continue;
      if (qp->validators[voter_index] != sender_pk)
        continue;
      if (!crypto::check_signature(bh, qp->validators[voter_index], sig))
        continue;
      ok = true;
      break;
    }

    if (!ok)
    {
      m.reply(arqma::pulse_wire::COMMAND_PULSE_VOTE,
          bt_dict{{arqma::pulse_wire::KEY_OK, 0LL},
              {arqma::pulse_wire::KEY_ERR, std::string{"signature or quorum binding failed"}}});
      return;
    }

    {
      std::string const pack = bh_s + sig_s + std::to_string(voter_index) + ":" + std::to_string(block_height);
      crypto::hash const vfp = crypto::cn_fast_hash(pack.data(), pack.size());
      if (g_pulse_seen_inbound.is_recent(vfp))
      {
        MDEBUG("pulse_vote duplicate (seen window), short-circuit");
        m.reply(arqma::pulse_wire::COMMAND_PULSE_VOTE,
            bt_dict{{arqma::pulse_wire::KEY_OK, 1LL},
                {arqma::pulse_wire::KEY_BLOCK_HASH, std::string{reinterpret_cast<const char *>(bh.data), sizeof(bh.data)}}});
        return;
      }
      g_pulse_seen_inbound.mark_seen(vfp);
    }

    cryptonote::pulse_validator_signature_entry pent{};
    pent.voter_index = voter_index;
    pent.signature = sig;
    std::memset(pent.padding, 0, sizeof(pent.padding));
    g_pulse_vote_accum.add(bh, block_height, voter_index, pent);

    MDEBUG("pulse_vote OK bh " << bh << " vi " << voter_index << " from " << as_hex(m.pubkey));
    m.reply(arqma::pulse_wire::COMMAND_PULSE_VOTE,
        bt_dict{{arqma::pulse_wire::KEY_OK, 1LL},
            {arqma::pulse_wire::KEY_BLOCK_HASH, std::string{reinterpret_cast<const char *>(bh.data), sizeof(bh.data)}}});

    {
      int64_t const rh_in = std::max(
          static_cast<int64_t>(0),
          get_or<int64_t>(d, arqma::pulse_wire::KEY_RELAY_HOPS, arqma::pulse_wire::DEFAULT_PULSE_RELAY_HOPS));
      if (rh_in > 0)
      {
        bt_dict fwd = d;
        fwd[arqma::pulse_wire::KEY_RELAY_HOPS] = static_cast<int64_t>(rh_in - 1);
        relay_pulse_quorum_peers(snw, block_height - 1, arqma::pulse_wire::COMMAND_PULSE_VOTE, fwd);
      }
    }
  }
  catch (const std::exception &e)
  {
    m.reply(arqma::pulse_wire::COMMAND_PULSE_VOTE,
        bt_dict{{arqma::pulse_wire::KEY_OK, 0LL}, {arqma::pulse_wire::KEY_ERR, std::string{e.what()}}});
  }
}

} // end empty namespace

void init_core_callbacks()
{
  cryptonote::arqnet_new = new_snnwrapper;
  cryptonote::arqnet_delete = delete_snnwrapper;
  cryptonote::arqnet_relay_obligation_votes = relay_obligation_votes;
  cryptonote::arqnet_pulse_vote_buffer_copy =
      [](void *, crypto::hash const &block_hash, uint64_t *height_out, std::vector<cryptonote::pulse_validator_signature_entry> *out_entries) -> bool {
        return g_pulse_vote_accum.copy(block_hash, height_out, out_entries);
      };
  cryptonote::arqnet_pulse_vote_buffer_clear = [](void *, crypto::hash const &block_hash) { g_pulse_vote_accum.clear(block_hash); };

  SNNetwork::register_command("vote_ob", SNNetwork::command_type::quorum, handle_obligation_vote);
  SNNetwork::register_command("ping", SNNetwork::command_type::public_, handle_ping);
  SNNetwork::register_command("pong", SNNetwork::command_type::public_, handle_pong);
  SNNetwork::register_command(arqma::pulse_wire::COMMAND_PULSE_CAP, SNNetwork::command_type::quorum, handle_pulse_cap);
  SNNetwork::register_command(arqma::pulse_wire::COMMAND_PULSE_PROPOSAL, SNNetwork::command_type::quorum, handle_pulse_proposal);
  SNNetwork::register_command(arqma::pulse_wire::COMMAND_PULSE_VOTE, SNNetwork::command_type::quorum, handle_pulse_vote);
}

} // arqnet
