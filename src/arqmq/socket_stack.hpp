// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "arqmq.h"
#include "transport.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>
#include <zmq.hpp>

namespace arqmq {
/// Inbound command framed for the dedicated ArqMQ socket stack.
struct InboundRequest
{
  std::string command;
  CategoryAcl peer_acl = CategoryAcl::Denied;
  std::string payload;
};

using CommandHandler = std::function<std::string(const InboundRequest&)>;

/// Dedicated ZMQ worker stack under Arqma naming (Milestone B).
///
/// Owns an independent `zmq::context_t`, inproc job sockets, and a worker
/// thread that enforces framing limits + ACL before invoking handlers.
/// Peer quorum mesh (`vote_ob` relay) continues via `arqnet::SNNetwork` until
/// dual-run cutover; this stack is the native ArqMQ transport path selected by
/// `--arqnet-backend=arqmq`.
class SocketStack final : public Transport
{
public:
  SocketStack();
  ~SocketStack() override;

  SocketStack(const SocketStack&) = delete;
  SocketStack& operator=(const SocketStack&) = delete;

  /// Starts the worker thread and inproc ZMQ endpoints.
  std::error_code start();

  /// Optional external bind (tcp/ipc). Must be called after `start()`.
  std::error_code bind(const std::string& endpoint);

  /// Registers or replaces a command handler. `required` is documented ACL;
  /// runtime authorize still uses `authorize_request` against `peer_acl`.
  void register_handler(std::string command, CategoryAcl required, CommandHandler handler);

  /// Submits a request through the worker and waits for a reply.
  std::error_code dispatch(const InboundRequest& request, std::string* reply);

  size_t handler_count() const;
  bool has_handler(std::string_view command) const;

  const char* name() const noexcept override;
  bool running() const noexcept override;
  void stop() noexcept override;

private:
  struct HandlerEntry
  {
    CategoryAcl required = CategoryAcl::Denied;
    CommandHandler handler;
  };

  struct Job
  {
    InboundRequest request;
    std::string* reply = nullptr;
    std::error_code* ec = nullptr;
    std::mutex* mu = nullptr;
    std::condition_variable* cv = nullptr;
    bool* done = nullptr;
  };

  void worker_main();
  std::error_code handle_job(const InboundRequest& request, std::string* reply);

  zmq::context_t context_;
  std::thread worker_;
  std::atomic<bool> running_{false};
  std::atomic<bool> stop_requested_{false};

  mutable std::mutex handlers_mu_;
  std::unordered_map<std::string, HandlerEntry> handlers_;

  std::mutex bind_mu_;
  std::vector<std::string> bind_endpoints_;
};
} // namespace arqmq
