// Copyright (c) 2018 - 2026, The Arqma Network

#pragma once

#include "etn_store.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

namespace arq_etn {

class EtNServer
{
public:
  EtNServer();
  ~EtNServer();

  EtNServer(const EtNServer&) = delete;
  EtNServer& operator=(const EtNServer&) = delete;

  void set_store(std::shared_ptr<EtNStore> store);
  void set_token(std::string token);
  void set_wallet_rpc_url(std::string url);

  std::error_code listen(const std::string& host, std::uint16_t port);
  void stop();

  std::uint16_t port() const noexcept;
  bool running() const noexcept;
  std::string base_url() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool parse_listen_address(const std::string& listen, std::string& host, std::uint16_t& port);

} // namespace arq_etn
