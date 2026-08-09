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

#include "socket_stack.hpp"

#include "command_registry.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace arqmq {
namespace {
constexpr const char* k_quit = "QUIT";
constexpr const char* k_bind = "BIND";

std::atomic<uint64_t> g_stack_id{1};

void send_pointer(zmq::socket_t& sock, void* ptr)
{
  const auto value = reinterpret_cast<uintptr_t>(ptr);
  zmq::message_t msg{sizeof(value)};
  std::memcpy(msg.data(), &value, sizeof(value));
  sock.send(msg, zmq::send_flags::none);
}

void* recv_pointer(zmq::socket_t& sock)
{
  zmq::message_t msg;
  const auto result = sock.recv(msg, zmq::recv_flags::none);
  if (!result || msg.size() != sizeof(uintptr_t))
    return nullptr;
  uintptr_t value = 0;
  std::memcpy(&value, msg.data(), sizeof(value));
  return reinterpret_cast<void*>(value);
}

std::string default_ping_handler(const InboundRequest&)
{
  return "pong";
}
} // namespace

SocketStack::SocketStack() : context_(1)
{
  const auto id = g_stack_id.fetch_add(1, std::memory_order_relaxed);
  jobs_endpoint_ = "inproc://arqmq.jobs." + std::to_string(id);
  ctrl_endpoint_ = "inproc://arqmq.ctrl." + std::to_string(id);
  register_handler("ping", CategoryAcl::Basic, default_ping_handler);
  register_handler("pong", CategoryAcl::Basic, [](const InboundRequest&) { return std::string{}; });
}

SocketStack::~SocketStack()
{
  stop();
}

std::error_code SocketStack::start()
{
  if (running_.load())
    return {};

  stop_requested_.store(false);
  try {
    worker_ = std::thread([this] { worker_main(); });
  } catch (...) {
    return std::make_error_code(std::errc::resource_unavailable_try_again);
  }

  // Wait for the worker to bind inproc endpoints (allow headroom on Debug CI).
  for (int i = 0; i < 500 && !running_.load(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

  if (!running_.load()) {
    stop();
    return std::make_error_code(std::errc::timed_out);
  }
  return {};
}

std::error_code SocketStack::bind(const std::string& endpoint)
{
  if (!running_.load() || endpoint.empty())
    return std::make_error_code(std::errc::invalid_argument);

  try {
    zmq::socket_t push{context_, zmq::socket_type::push};
    push.connect(ctrl_endpoint_);
    zmq::message_t cmd{k_bind, std::strlen(k_bind)};
    push.send(cmd, zmq::send_flags::sndmore);
    zmq::message_t ep{endpoint.data(), endpoint.size()};
    push.send(ep, zmq::send_flags::none);
  } catch (...) {
    return std::make_error_code(std::errc::io_error);
  }

  for (int i = 0; i < 500; ++i) {
    {
      std::lock_guard<std::mutex> lock{bind_mu_};
      for (const auto& bound : bind_endpoints_) {
        if (bound == endpoint)
          return {};
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return std::make_error_code(std::errc::io_error);
}

void SocketStack::register_handler(std::string command, CategoryAcl required, CommandHandler handler)
{
  std::lock_guard<std::mutex> lock{handlers_mu_};
  handlers_[std::move(command)] = HandlerEntry{required, std::move(handler)};
}

std::error_code SocketStack::dispatch(const InboundRequest& request, std::string* reply)
{
  if (!running_.load())
    return std::make_error_code(std::errc::not_connected);

  std::error_code ec;
  std::string local_reply;
  std::string* reply_ptr = reply ? reply : &local_reply;
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  Job job{request, reply_ptr, &ec, &mu, &cv, &done};

  try {
    zmq::socket_t push{context_, zmq::socket_type::push};
    push.connect(jobs_endpoint_);
    send_pointer(push, &job);
  } catch (...) {
    return std::make_error_code(std::errc::io_error);
  }

  std::unique_lock<std::mutex> lock{mu};
  if (!cv.wait_for(lock, std::chrono::seconds(5), [&] { return done; }))
    return std::make_error_code(std::errc::timed_out);
  return ec;
}

size_t SocketStack::handler_count() const
{
  std::lock_guard<std::mutex> lock{handlers_mu_};
  return handlers_.size();
}

bool SocketStack::has_handler(std::string_view command) const
{
  std::lock_guard<std::mutex> lock{handlers_mu_};
  return handlers_.find(std::string{command}) != handlers_.end();
}

const char* SocketStack::name() const noexcept
{
  return k_transport_arqmq;
}

bool SocketStack::running() const noexcept
{
  return running_.load();
}

void SocketStack::stop() noexcept
{
  if (!worker_.joinable()) {
    running_.store(false);
    return;
  }

  stop_requested_.store(true);
  try {
    zmq::socket_t push{context_, zmq::socket_type::push};
    push.connect(ctrl_endpoint_);
    zmq::message_t quit{k_quit, std::strlen(k_quit)};
    push.send(quit, zmq::send_flags::none);
  } catch (...) {
    // Fall through to join; worker may already be exiting.
  }

  if (worker_.joinable())
    worker_.join();
  running_.store(false);
  stop_requested_.store(false);

  std::lock_guard<std::mutex> lock{bind_mu_};
  bind_endpoints_.clear();
}

std::error_code SocketStack::handle_job(const InboundRequest& request, std::string* reply)
{
  if (!authorize_request(request.command, request.peer_acl, request.payload.size(), 1))
    return std::make_error_code(std::errc::permission_denied);

  HandlerEntry entry;
  {
    std::lock_guard<std::mutex> lock{handlers_mu_};
    const auto it = handlers_.find(request.command);
    if (it == handlers_.end() || !it->second.handler)
      return std::make_error_code(std::errc::operation_not_supported);
    entry = it->second;
  }

  try {
    const std::string out = entry.handler(request);
    if (reply)
      *reply = out;
    return {};
  } catch (...) {
    return std::make_error_code(std::errc::io_error);
  }
}

void SocketStack::worker_main()
{
  zmq::socket_t jobs{context_, zmq::socket_type::pull};
  zmq::socket_t ctrl{context_, zmq::socket_type::pull};
  std::vector<std::unique_ptr<zmq::socket_t>> listeners;

  try {
    jobs.bind(jobs_endpoint_);
    ctrl.bind(ctrl_endpoint_);
  } catch (...) {
    running_.store(false);
    return;
  }

  running_.store(true);

  while (!stop_requested_.load()) {
    zmq::pollitem_t items[] = {
        {jobs.handle(), 0, ZMQ_POLLIN, 0},
        {ctrl.handle(), 0, ZMQ_POLLIN, 0},
    };
    zmq::poll(items, 2, std::chrono::milliseconds{100});

    if (items[1].revents & ZMQ_POLLIN) {
      zmq::message_t cmd;
      if (!ctrl.recv(cmd, zmq::recv_flags::none))
        continue;
      const std::string command{static_cast<char*>(cmd.data()), cmd.size()};
      if (command == k_quit)
        break;
      if (command == k_bind) {
        zmq::message_t ep_msg;
        if (!ctrl.recv(ep_msg, zmq::recv_flags::none))
          continue;
        const std::string endpoint{static_cast<char*>(ep_msg.data()), ep_msg.size()};
        try {
          auto sock = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::router);
          sock->set(zmq::sockopt::linger, 0);
          sock->bind(endpoint);
          listeners.push_back(std::move(sock));
          std::lock_guard<std::mutex> lock{bind_mu_};
          bind_endpoints_.push_back(endpoint);
        } catch (...) {
          // Leave endpoint unrecorded; bind() times out.
        }
      }
    }

    if (items[0].revents & ZMQ_POLLIN) {
      auto* raw = recv_pointer(jobs);
      auto* job = static_cast<Job*>(raw);
      if (!job)
        continue;
      std::error_code ec = handle_job(job->request, job->reply);
      if (job->ec)
        *job->ec = ec;
      if (job->mu && job->cv && job->done) {
        std::lock_guard<std::mutex> lock{*job->mu};
        *job->done = true;
        job->cv->notify_one();
      }
    }
  }

  listeners.clear();
  running_.store(false);
}
} // namespace arqmq
