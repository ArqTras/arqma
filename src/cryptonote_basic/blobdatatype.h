// Copyright (c) 2018 - 2026, The Arqma Network
//
// Thin compatibility shim: older tests/fuzz code expected this header and
// cryptonote::blobdata. Serialization uses std::string for binary blobs.

#pragma once

#include <string>

namespace cryptonote
{
  using blobdata = std::string;
}
