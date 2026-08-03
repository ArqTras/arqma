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

#include "message_envelope.hpp"

namespace arq_messaging
{
  /// Soft upper bound for envelope TTL until a production storage policy lands.
  constexpr std::uint32_t max_envelope_ttl_seconds = 14 * 24 * 60 * 60;

  inline bool validate_envelope_ttl(const MessageEnvelope &envelope) noexcept
  {
    return envelope.ttl_seconds > 0 && envelope.ttl_seconds <= max_envelope_ttl_seconds;
  }

  /// Placeholder sealed-sender metadata. Real sender-hiding crypto is deferred;
  /// this only tags envelopes that opt into the future sealed path.
  struct SealedSenderTag
  {
    static constexpr std::uint8_t marker = 0xa1;
    bool enabled = false;
  };

  inline MessageEnvelope with_sealed_sender_marker(MessageEnvelope envelope, bool enable)
  {
    if (enable && !envelope.payload.empty() && envelope.payload.front() != SealedSenderTag::marker)
    {
      envelope.payload.insert(envelope.payload.begin(), SealedSenderTag::marker);
    }
    return envelope;
  }

  inline bool has_sealed_sender_marker(const MessageEnvelope &envelope) noexcept
  {
    return !envelope.payload.empty() && envelope.payload.front() == SealedSenderTag::marker;
  }
}
