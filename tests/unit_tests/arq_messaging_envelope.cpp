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

#include "arq_messaging/message_envelope.hpp"

TEST(arq_messaging_envelope, roundtrip_binary_encoding)
{
  arq_messaging::MessageEnvelope envelope{};
  envelope.version = arq_messaging::MessageEnvelope::current_version;
  envelope.ttl_seconds = 180;
  envelope.payload = {0xde, 0xad, 0xbe, 0xef};

  for (std::size_t i = 0; i < envelope.recipient_x25519.data.size(); ++i)
    envelope.recipient_x25519.data[i] = static_cast<std::uint8_t>(i);

  const auto encoded = arq_messaging::encode_message_envelope(envelope);

  arq_messaging::MessageEnvelope decoded{};
  ASSERT_TRUE(arq_messaging::decode_message_envelope(
      std::string_view{reinterpret_cast<const char *>(encoded.data()), encoded.size()}, decoded));

  EXPECT_EQ(envelope.version, decoded.version);
  EXPECT_EQ(envelope.recipient_x25519.data, decoded.recipient_x25519.data);
  EXPECT_EQ(envelope.ttl_seconds, decoded.ttl_seconds);
  EXPECT_EQ(envelope.payload, decoded.payload);
}

TEST(arq_messaging_envelope, rejects_truncated_payload)
{
  arq_messaging::MessageEnvelope envelope{};
  envelope.payload = {1, 2, 3};

  auto encoded = arq_messaging::encode_message_envelope(envelope);
  encoded.pop_back();

  arq_messaging::MessageEnvelope decoded{};
  EXPECT_FALSE(arq_messaging::decode_message_envelope(
      std::string_view{reinterpret_cast<const char *>(encoded.data()), encoded.size()}, decoded));
}
