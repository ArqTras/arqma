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

TEST(arq_storage_client, remote_backend_reports_not_connected)
{
  arq_storage::StorageClient client{arq_storage::Backend::Remote};
  const auto expected = std::make_error_code(std::errc::not_connected);

  EXPECT_EQ(expected, client.ping());
  EXPECT_EQ(expected, client.store({"messages", "hello", "world"}));

  const auto retrieve = client.retrieve("messages", "hello");
  EXPECT_EQ(expected, retrieve.error);
  EXPECT_TRUE(retrieve.value.empty());
}

TEST(arq_storage_client, in_memory_store_retrieve_roundtrip)
{
  arq_storage::StorageClient client{arq_storage::Backend::InMemory};
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
