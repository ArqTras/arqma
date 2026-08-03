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

#include "arqmq.h"

#include <system_error>

namespace
{
  constexpr auto not_supported = std::errc::function_not_supported;
}

namespace arqmq
{
  const char *to_string(const Backend backend) noexcept
  {
    switch (backend)
    {
      case Backend::LegacyArqNet:
        return "legacy-arqnet";
      case Backend::ArqMq:
        return "arqmq";
    }

    return "unknown";
  }

  const char *to_string(const CategoryAcl acl) noexcept
  {
    switch (acl)
    {
      case CategoryAcl::Denied:
        return "denied";
      case CategoryAcl::Basic:
        return "basic";
      case CategoryAcl::ServiceNode:
        return "service-node";
      case CategoryAcl::Admin:
        return "admin";
    }

    return "unknown";
  }

  std::error_code init(const Config &config) noexcept
  {
    switch (config.backend)
    {
      case Backend::LegacyArqNet:
        return {};
      case Backend::ArqMq:
        // TODO(arqma): implement the native ArqMQ backend under Arqma naming.
        return std::make_error_code(not_supported);
    }

    return std::make_error_code(std::errc::invalid_argument);
  }

  std::error_code shutdown() noexcept
  {
    // TODO(arqma): coordinate shutdown across the future dual-backend facade.
    return {};
  }
}
