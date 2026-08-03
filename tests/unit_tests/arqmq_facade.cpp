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

#include "arqmq/arqmq.h"

TEST(arqmq_facade, backend_names_match_scaffold)
{
  EXPECT_STREQ("legacy-arqnet", arqmq::to_string(arqmq::Backend::LegacyArqNet));
  EXPECT_STREQ("arqmq", arqmq::to_string(arqmq::Backend::ArqMq));
}

TEST(arqmq_facade, acl_names_match_scaffold)
{
  EXPECT_STREQ("denied", arqmq::to_string(arqmq::CategoryAcl::Denied));
  EXPECT_STREQ("basic", arqmq::to_string(arqmq::CategoryAcl::Basic));
  EXPECT_STREQ("service-node", arqmq::to_string(arqmq::CategoryAcl::ServiceNode));
  EXPECT_STREQ("admin", arqmq::to_string(arqmq::CategoryAcl::Admin));
}

TEST(arqmq_facade, legacy_backend_initializes)
{
  const arqmq::Config config{arqmq::Backend::LegacyArqNet, arqmq::CategoryAcl::ServiceNode};
  EXPECT_FALSE(arqmq::init(config));
  EXPECT_FALSE(arqmq::shutdown());
}

TEST(arqmq_facade, native_backend_reports_not_supported)
{
  const arqmq::Config config{arqmq::Backend::ArqMq, arqmq::CategoryAcl::Admin};
  EXPECT_EQ(std::make_error_code(std::errc::function_not_supported), arqmq::init(config));
}
