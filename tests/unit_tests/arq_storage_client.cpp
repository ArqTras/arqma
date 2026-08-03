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

#include "gtest/gtest.h"

#include "arq_storage/storage_client.h"
#include "arq_storage/storage_endpoint.h"

TEST(arq_storage_endpoint, parses_http_and_https_urls)
{
  const auto http = arq_storage::parse_endpoint("http://127.0.0.1:22021/status");
  ASSERT_TRUE(http);
  EXPECT_FALSE(http.tls);
  EXPECT_EQ("127.0.0.1", http.host);
  EXPECT_EQ(22021, http.port);
  EXPECT_EQ("/status", http.path);

  const auto https = arq_storage::parse_endpoint("https://storage.example");
  ASSERT_TRUE(https);
  EXPECT_TRUE(https.tls);
  EXPECT_EQ("storage.example", https.host);
  EXPECT_EQ(443, https.port);
  EXPECT_EQ("/", https.path);
}

TEST(arq_storage_endpoint, rejects_invalid_urls)
{
  EXPECT_FALSE(arq_storage::parse_endpoint(""));
  EXPECT_FALSE(arq_storage::parse_endpoint("ftp://127.0.0.1:1"));
  EXPECT_FALSE(arq_storage::parse_endpoint("http://"));
  EXPECT_FALSE(arq_storage::parse_endpoint("http://host:99999"));
}

TEST(arq_storage_endpoint, formats_http_get_request)
{
  const auto ep = arq_storage::parse_endpoint("http://127.0.0.1:22021/status");
  ASSERT_TRUE(ep);
  const auto req = arq_storage::format_http_get_request(ep);
  EXPECT_NE(std::string::npos, req.find("GET /status HTTP/1.1\r\n"));
  EXPECT_NE(std::string::npos, req.find("Host: 127.0.0.1\r\n"));
  EXPECT_NE(std::string::npos, req.find("Connection: close\r\n\r\n"));
}

TEST(arq_storage_client, remote_without_url_is_not_connected)
{
  arq_storage::StorageClient client{{arq_storage::Backend::Remote, "", std::chrono::milliseconds{100}}};
  EXPECT_EQ(std::make_error_code(std::errc::not_connected), client.ping());
}

TEST(arq_storage_client, remote_with_closed_port_is_not_connected)
{
  // Port 1 is almost never accepting connections on developer workstations.
  arq_storage::Config cfg{arq_storage::Backend::Remote, "http://127.0.0.1:1", std::chrono::milliseconds{200}};
  arq_storage::StorageClient client{cfg};
  ASSERT_TRUE(client.endpoint());
  EXPECT_EQ(std::make_error_code(std::errc::not_connected), client.ping());
}

TEST(arq_storage_client, in_memory_store_retrieve_roundtrip)
{
  arq_storage::StorageClient client{{arq_storage::Backend::InMemory, "", std::chrono::milliseconds{100}}};
  EXPECT_FALSE(client.ping());
  EXPECT_FALSE(client.store({"messages", "hello", "world"}));

  const auto retrieve = client.retrieve("messages", "hello");
  EXPECT_FALSE(retrieve.error);
  EXPECT_EQ("world", retrieve.value);

  client.set_snodes_for_pubkey("pk", {"sn-a", "sn-b"});
  const auto snodes = client.get_snodes_for_pubkey("pk");
  ASSERT_FALSE(snodes.error);
  ASSERT_EQ(2u, snodes.value.size());
  EXPECT_EQ("sn-a", snodes.value[0]);
}

TEST(arq_storage_client, daemon_client_config_roundtrip)
{
  arq_storage::configure_daemon_client({arq_storage::Backend::InMemory, "", std::chrono::milliseconds{50}});
  auto client = arq_storage::daemon_client();
  EXPECT_EQ(arq_storage::Backend::InMemory, client.backend());
  EXPECT_FALSE(client.ping());
  arq_storage::configure_daemon_client({});
}
