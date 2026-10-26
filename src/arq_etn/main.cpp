// Copyright (c) 2018 - 2026, The Arqma Network

#include "etn_server.h"
#include "etn_store.h"

#include <boost/program_options.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

int main(int argc, char** argv)
{
  namespace po = boost::program_options;
  std::string listen = "127.0.0.1:22050";
  std::string data_dir;
  std::string token;
  std::string wallet_rpc;
  std::string issuer_id = "arqma-etn-issuer";
  po::options_description desc{"arqma-etn-audit"};
  desc.add_options()("help,h", "show help")(
      "listen", po::value<std::string>(&listen)->default_value(listen), "host:port")(
      "data-dir", po::value<std::string>(&data_dir), "file-backed reserve + attestation store")(
      "token", po::value<std::string>(&token), "optional API token (or ARQMA_STACK_TOKEN)")(
      "wallet-rpc-url", po::value<std::string>(&wallet_rpc),
      "arqma-wallet-rpc JSON-RPC URL for get_reserve_proof (optional)")(
      "issuer-id", po::value<std::string>(&issuer_id)->default_value(issuer_id), "issuer identifier in PoR packages");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc
              << "\nETN model A/C companion (not consensus). Docs: docs/ETN_OPERATOR.md\n"
                 "Example:\n  arqma-etn-audit --listen 127.0.0.1:22050 --data-dir ~/.arqma/etn-audit\n";
    return 0;
  }
  std::string host;
  std::uint16_t port = 0;
  if (!arq_etn::parse_listen_address(listen, host, port)) {
    std::cerr << "listen must be host:port\n";
    return 1;
  }
  if (token.empty()) {
    if (const char* env = std::getenv("ARQMA_STACK_TOKEN"); env && *env)
      token = env;
  }
  auto store = std::make_shared<arq_etn::EtNStore>();
  store->set_issuer_id(issuer_id);
  if (!data_dir.empty())
    store->set_data_dir(data_dir);
  arq_etn::EtNServer server;
  server.set_store(store);
  if (!token.empty())
    server.set_token(token);
  if (!wallet_rpc.empty())
    server.set_wallet_rpc_url(wallet_rpc);
  if (const auto ec = server.listen(host, port)) {
    std::cerr << "bind failed: " << ec.message() << "\n";
    return 1;
  }
  std::cout << "arqma-etn-audit listening on " << server.base_url() << std::endl;
  while (server.running())
    std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}
