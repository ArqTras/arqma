// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "arq_messaging/identity.hpp"
#include "arq_messaging/message_envelope.hpp"
#include "arq_messaging/onion_layer.hpp"
#include "arq_messaging/onion_request.hpp"
#include "arq_messaging/sealed_sender.hpp"
#include "arq_messaging/stack_env.hpp"
#include "arq_storage/http_io.h"
#include "arq_storage/storage_client.h"

#include <boost/program_options.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::string to_hex(const std::uint8_t* data, std::size_t n)
{
  static const char* digits = "0123456789abcdef";
  std::string out(n * 2, '0');
  for (std::size_t i = 0; i < n; ++i) {
    out[i * 2] = digits[data[i] >> 4];
    out[i * 2 + 1] = digits[data[i] & 0xf];
  }
  return out;
}

bool from_hex(const std::string& hex, std::uint8_t* out, std::size_t n)
{
  if (hex.size() != n * 2)
    return false;
  auto nib = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
      return 10 + (c - 'A');
    return -1;
  };
  for (std::size_t i = 0; i < n; ++i) {
    const int hi = nib(hex[i * 2]);
    const int lo = nib(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0)
      return false;
    out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return true;
}
} // namespace

int main(int argc, char** argv)
{
  namespace po = boost::program_options;
  std::string cmd;
  std::string url = "http://127.0.0.1:22021";
  std::string to;
  std::string text;
  std::string key;
  std::string secret;
  std::vector<std::string> routers;
  std::vector<std::string> snodes;
  std::uint32_t ttl = 3600;
  po::options_description desc{"arqma-msg"};
  desc.add_options()("help,h", "show help")("url", po::value<std::string>(&url), "storage base URL")(
      "to", po::value<std::string>(&to), "recipient x25519 hex (send)")("ttl", po::value<std::uint32_t>(&ttl),
                                                                        "envelope TTL seconds")(
      "key", po::value<std::string>(&key), "storage key (get/open)")("text", po::value<std::string>(&text),
                                                                     "plaintext (send)")(
      "secret", po::value<std::string>(&secret),
      "recipient private key hex (open)")("router", po::value<std::vector<std::string>>(&routers)->composing(),
                                          "arqma-router URL (repeat, outermost first, max 3)")(
      "snode", po::value<std::vector<std::string>>(&snodes)->composing(), "storage member URL (swarm announce)");
  po::options_description hidden;
  hidden.add_options()("cmd", po::value<std::string>(&cmd));
  po::positional_options_description pos;
  pos.add("cmd", 1);
  po::variables_map vm;
  po::options_description all;
  all.add(desc).add(hidden);
  po::store(po::command_line_parser(argc, argv).options(all).positional(pos).run(), vm);
  po::notify(vm);
  const auto stack = arq_messaging::load_stack_env();
  if (!vm.count("url"))
    url = stack.storage_url;
  const bool explicit_router = vm.count("router") != 0;
  if (!explicit_router && !stack.router_url.empty())
    routers.push_back(stack.router_url);
  if (vm.count("help") || cmd.empty()) {
    std::cout << "Usage: arqma-msg gen|send|inbox|open [options]\n"
              << desc
              << "\nEveryday (start utils/arqma-stack.sh first):\n"
                 "  arqma-msg gen\n"
                 "  arqma-msg send --to <hex> --text hello\n"
                 "  arqma-msg inbox --to <hex>\n"
                 "  arqma-msg open --to <pub> --secret <priv> --key <id>\n"
                 "\nOperator knobs: --url, --router (repeat, max 3), swarm --snode.\n";
    return vm.count("help") ? 0 : 1;
  }

  if (cmd == "gen") {
    arq_messaging::Identity id{};
    if (arq_messaging::generate_identity(id))
      return 1;
    std::cout << to_hex(id.public_key.data.data(), 32) << " " << to_hex(id.private_key.data.data(), 32) << "\n";
    return 0;
  }

  arq_storage::Config cfg;
  cfg.backend = arq_storage::Backend::Remote;
  cfg.base_url = url;
  cfg.connect_timeout = std::chrono::milliseconds{2000};
  arq_storage::StorageClient client{cfg};

  if (cmd == "send") {
    if (to.size() != 64 || text.empty()) {
      std::cerr << "send requires --to <64 hex> and --text\n";
      return 1;
    }
    arq_messaging::X25519PublicKey pub{};
    if (!from_hex(to, pub.data.data(), 32))
      return 1;
    std::vector<std::uint8_t> plain(text.begin(), text.end());
    std::vector<std::uint8_t> sealed;
    if (arq_messaging::seal_payload(pub, plain, sealed)) {
      std::cerr << "seal failed\n";
      return 1;
    }
    arq_messaging::MessageEnvelope env{};
    env.recipient_x25519 = pub;
    env.ttl_seconds = ttl == 0 ? 3600 : ttl;
    env.payload = std::move(sealed);
    const auto blob = arq_messaging::encode_message_envelope(env);
    const auto store_key = to_hex(blob.data(), std::min<std::size_t>(blob.size(), 16));
    if (explicit_router && routers.size() > arq_messaging::OnionRequest::hop_count) {
      std::cerr << "at most 3 --router hops\n";
      return 1;
    }
    auto send_via_router = [&]() -> bool {
      if (routers.size() > arq_messaging::OnionRequest::hop_count)
        return false;
      std::vector<arq_messaging::X25519PublicKey> pubs;
      pubs.reserve(routers.size());
      for (const auto& router : routers) {
        const auto ep = arq_storage::parse_endpoint(router);
        const auto pubget = arq_storage::http_exchange(ep, "GET", "/v1/pubkey", {});
        if (!pubget || pubget.body.size() < 64)
          return false;
        arq_messaging::X25519PublicKey rpub{};
        if (!from_hex(pubget.body.substr(0, 64), rpub.data.data(), 32))
          return false;
        pubs.push_back(rpub);
      }
      std::vector<std::uint8_t> onion;
      if (arq_messaging::compose_onion_route(pubs, routers, blob, onion))
        return false;
      std::string rpath = "/v1/store?ns=inbox-" + to + "&key=" + store_key;
      if (env.ttl_seconds != 0)
        rpath += "&ttl=" + std::to_string(env.ttl_seconds);
      const auto ep = arq_storage::parse_endpoint(routers.front());
      return static_cast<bool>(arq_storage::http_exchange(ep, "POST", rpath, std::string(onion.begin(), onion.end())));
    };
    if (!routers.empty()) {
      if (send_via_router()) {
        std::cout << store_key << "\n";
        return 0;
      }
      if (explicit_router) {
        std::cerr << "router store failed\n";
        return 1;
      }
    }
    arq_storage::StoreRequest req{"inbox-" + to, store_key, std::string(blob.begin(), blob.end())};
    req.ttl_seconds = env.ttl_seconds;
    if (const auto ec = client.store(req)) {
      std::cerr << "store failed: " << ec.message() << "\n";
      return 1;
    }
    std::cout << store_key << "\n";
    return 0;
  }

  if (cmd == "get") {
    if (to.size() != 64 || key.empty()) {
      std::cerr << "get requires --to <recipient hex> and --key\n";
      return 1;
    }
    const auto got = client.retrieve("inbox-" + to, key);
    if (!got) {
      std::cerr << "retrieve failed: " << got.error.message() << "\n";
      return 1;
    }
    std::cout << got.value << "\n";
    return 0;
  }

  if (cmd == "inbox") {
    if (to.size() != 64) {
      std::cerr << "inbox requires --to <recipient hex>\n";
      return 1;
    }
    const auto keys = client.list_keys("inbox-" + to);
    if (!keys) {
      std::cerr << "list failed: " << keys.error.message() << "\n";
      return 1;
    }
    for (const auto& k : keys.value)
      std::cout << k << "\n";
    return 0;
  }

  if (cmd == "open") {
    if (to.size() != 64 || secret.size() != 64 || key.empty()) {
      std::cerr << "open requires --to <pub hex> --secret <priv hex> --key\n";
      return 1;
    }
    const auto got = client.retrieve("inbox-" + to, key);
    if (!got) {
      std::cerr << "retrieve failed: " << got.error.message() << "\n";
      return 1;
    }
    arq_messaging::MessageEnvelope env{};
    if (!arq_messaging::decode_message_envelope(got.value, env)) {
      std::cerr << "bad envelope\n";
      return 1;
    }
    arq_messaging::Identity id{};
    if (!from_hex(to, id.public_key.data.data(), 32) || !from_hex(secret, id.private_key.data.data(), 32))
      return 1;
    std::vector<std::uint8_t> plain;
    if (arq_messaging::open_payload(id, env.payload, plain)) {
      std::cerr << "open failed\n";
      return 1;
    }
    std::cout << std::string(plain.begin(), plain.end()) << "\n";
    return 0;
  }

  if (cmd == "swarm") {
    if (to.size() != 64) {
      std::cerr << "swarm requires --to <recipient hex>\n";
      return 1;
    }
    if (!snodes.empty())
      client.set_snodes_for_pubkey(to, snodes);
    const auto got = client.get_swarm(to);
    if (!got) {
      std::cerr << "swarm failed: " << got.error.message() << "\n";
      return 1;
    }
    std::cout << got.value.first << "\n";
    for (const auto& member : got.value.second)
      std::cout << member << "\n";
    return 0;
  }

  std::cerr << "unknown command " << cmd << "\n";
  return 1;
}
