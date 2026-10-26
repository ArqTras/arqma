#pragma once
#include <cstdint>
#include <array>

// Software triple (major, minor, revision) for SN / RPC wire compatibility.
extern const std::array<uint16_t, 3> ARQMA_VERSION;
// Fourth version component (build number).
extern const uint32_t ARQMA_VERSION_BUILD;

extern const char* const ARQMA_VERSION_TAG;
extern const char* const ARQMA_VERSION_STR;
extern const char* const ARQMA_RELEASE_NAME;
extern const char* const ARQMA_VERSION_FULL;

extern const bool ARQMA_VERSION_IS_RELEASE;
