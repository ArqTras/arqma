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

#include "backend_policy.hpp"

namespace arqmq {
BackendSelection resolve_backend(const std::string_view requested, const NetworkClass network,
                                 const bool allow_experimental_on_mainnet)
{
  BackendSelection out{};

  if (requested.empty() || requested == "legacy-arqnet") {
    out.backend = Backend::LegacyArqNet;
    out.reason = "legacy-arqnet (production default)";
    return out;
  }

  if (requested == "arqmq") {
    if (network == NetworkClass::Mainnet && !allow_experimental_on_mainnet) {
      out.backend = Backend::LegacyArqNet;
      out.overridden = true;
      out.reason = "arqmq refused on mainnet: keep legacy-arqnet until dual-run cutover; "
                   "peer mesh stays on SNNetwork for compatibility";
      return out;
    }
    out.backend = Backend::ArqMq;
    out.reason = (network == NetworkClass::Mainnet) ? "arqmq allowed on mainnet via explicit experimental override "
                                                      "(peer mesh still SNNetwork)"
                                                    : "arqmq experimental on non-mainnet (peer mesh still SNNetwork)";
    return out;
  }

  out.backend = Backend::LegacyArqNet;
  out.overridden = true;
  out.reason = "unknown --arqnet-backend value; falling back to legacy-arqnet";
  return out;
}
} // namespace arqmq
