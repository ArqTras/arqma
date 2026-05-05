// Copyright (c) 2026, The Arqma Network
//
// Pulse-over-arqnet (ZMQ/bt-dict): command names and payload keys for SN-quorum wires.
// Does not bypass P2P or mempool acceptance — daemons validate and may use payloads for tooling / relay.

#pragma once

#include <cstddef>

namespace arqma::pulse_wire {

/** Capability ping; reply echoes tag (see handlers). */
inline constexpr char COMMAND_PULSE_CAP[] = "pulse_cap";

/** Block blob proposal: request `blk`; reply `ok` + `bh` (32-byte hash) or `ok`=0 + `e`. */
inline constexpr char COMMAND_PULSE_PROPOSAL[] = "pulse_proposal";

/**
 * Single validator Pulse vote over `bh` at checkpointing quorum for parent height `block_height - 1`.
 * Request: `bh` (32-byte), `h` block height (signed block), `vi` voter_index, `sig` (binary `crypto::signature`).
 * Reply: `ok` + echoed `bh` or `ok`=0 + `e`.
 */
inline constexpr char COMMAND_PULSE_VOTE[] = "pulse_vote";

inline constexpr char KEY_TAG[] = "!";
inline constexpr char KEY_OK[] = "ok";
inline constexpr char KEY_ERR[] = "e";
inline constexpr char KEY_WIRE_VERSION[] = "wv";
inline constexpr char KEY_FORK_ACTIVE[] = "fa";
inline constexpr char KEY_CHAIN_HEIGHT[] = "h";       // pulse_cap & pulse_vote context
inline constexpr char KEY_BLOCK_BLOB[] = "blk";
inline constexpr char KEY_BLOCK_HASH[] = "bh";
inline constexpr char KEY_VOTER_INDEX[] = "vi";
inline constexpr char KEY_SIGNATURE[] = "sig";
inline constexpr char KEY_IDEAL_HF[] = "ihf";
inline constexpr char KEY_PULSE_MAJOR_READY[] = "pmr";

inline constexpr unsigned WIRE_VERSION_V1 = 1;

/** Soft cap for proposal blob (must stay under ZMQ max message size). */
inline constexpr std::size_t PULSE_PROPOSAL_MAX_BLOB_BYTES = 1000 * 1000;

/**
 * Relay hop countdown for **`pulse_proposal`** / **`pulse_vote`** gossip (integer ≥ 0).
 * Omitted ⇒ **`DEFAULT_PULSE_RELAY_HOPS`** on receive; forwarded payload uses **`rh` − 1** until 0 (no further relay).
 */
inline constexpr char KEY_RELAY_HOPS[] = "rh";
inline constexpr int64_t DEFAULT_PULSE_RELAY_HOPS = 3;

/**
 * Per-sender (hex ZMQ pubkey) flood guard for **`pulse_proposal`** + **`pulse_vote`** combined on one daemon.
 * Tweak for rehearsal if legitimate bursts hit the cap (`arqnet.cpp`).
 */
inline constexpr std::size_t PULSE_QUORUM_PEER_RATE_MAX_EVENTS = 48;
inline constexpr unsigned PULSE_QUORUM_PEER_RATE_WINDOW_SEC = 10;

} // namespace arqma::pulse_wire
