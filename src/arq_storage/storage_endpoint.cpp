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

#include "storage_endpoint.h"

#include <cctype>
#include <cstdlib>

namespace arq_storage
{
  Endpoint parse_endpoint(const std::string_view url) noexcept
  {
    Endpoint out{};
    if (url.empty())
      return out;

    std::string_view rest = url;
    if (rest.size() >= 8 && rest.substr(0, 8) == "https://")
    {
      out.tls = true;
      rest.remove_prefix(8);
    }
    else if (rest.size() >= 7 && rest.substr(0, 7) == "http://")
    {
      out.tls = false;
      rest.remove_prefix(7);
    }
    else
      return {};

    const auto slash = rest.find('/');
    const std::string_view hostport = slash == std::string_view::npos ? rest : rest.substr(0, slash);
    if (slash != std::string_view::npos)
      out.path = std::string{rest.substr(slash)};
    if (out.path.empty())
      out.path = "/";

    if (hostport.empty() || hostport.find(' ') != std::string_view::npos)
      return {};

    const auto colon = hostport.rfind(':');
    if (colon == std::string_view::npos)
    {
      out.host = std::string{hostport};
      out.port = out.tls ? 443 : 80;
    }
    else
    {
      if (colon == 0)
        return {};
      out.host = std::string{hostport.substr(0, colon)};
      const auto port_sv = hostport.substr(colon + 1);
      if (port_sv.empty())
        return {};
      for (char c : port_sv)
      {
        if (!std::isdigit(static_cast<unsigned char>(c)))
          return {};
      }
      const long port = std::strtol(std::string{port_sv}.c_str(), nullptr, 10);
      if (port <= 0 || port > 65535)
        return {};
      out.port = static_cast<std::uint16_t>(port);
    }

    if (out.host.empty() || out.host == "." || out.host.find('/') != std::string::npos)
      return {};
    return out;
  }
}
