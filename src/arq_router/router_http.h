// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#pragma once

#include "arq_messaging/identity.hpp"

#include <string>

namespace arq_router {
/// HTTP handler for the `arqma-router` process (status + one onion peel).
std::string handle_http(const std::string& method, const std::string& path, const std::string& body,
                        const arq_messaging::Identity& hop);
} // namespace arq_router
