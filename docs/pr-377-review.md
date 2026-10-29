# Review of arqma/arqma#377 (`upgrade` → `master`)

Reviewed tip: `0d6db6b20e00e6a8cbc2eb876b395ad48c499d8a`  
Base: `arqma/arqma` `master` at `4b067e8c22d1feeab3f78c848c44fc24a1bcdd0e` (`Hf_19 CLSAG + Atomic Swap`)  
Diff size: 175 files, +20863 / −4045.

This note is a code review of that tip. It does not change hard-fork heights, Pulse stage, or the default Arq-Net backend. Safe fixes that landed with this note are listed at the end. Follow-up: https://github.com/ArqTras/arqma/pull/7

`mergeable` on GitHub is `MERGEABLE`. `mergeStateStatus` is `BLOCKED` because `reviewDecision` is `REVIEW_REQUIRED`, and because `macos-14-unit-Release` failed. That macOS job died in **Install dependencies (macOS)** (Homebrew), before configure. Ubuntu unit Release and Debug on the same tip completed successfully. `linux-asan-ubsan` was skipped after the macOS failure.

## Architecture

| Layer | What the branch adds | When it affects mainnet |
| --- | --- | --- |
| Hard forks | HF20 hybrid window, HF21 exclusive-mesh version | HF19 height unchanged at 1 886 030. HF20 at 4 000 000. HF21 at 5 000 000. |
| Pulse | Optional miner-tx certificate (`tx_extra` tag), quorum collector, `pulse_rnd` gossip | Rules run only when `hard_fork_version >= 20`. Missing certificate does not fail the block. |
| PoW | RandomARQ still checked | `pow_required_for_block` returns true at every version. `pow_replacement_ready()` returns false. `k_pulse_pow_stage` is 2. |
| Arq-Net | `legacy-arqnet` (SNNetwork) or experimental `arqmq` SocketStack | Default is legacy. Mainnet `arqmq` without `--arqnet-allow-experimental` is forced back to legacy. Live quorum stays on SNNetwork until HF20 **and** a CURVE stack is up. |
| Shadow mesh | Opt-in dual-write (`--arqnet-mesh-shadow`) | Off by default. Mainnet also needs the experimental flag. Does not replace SNNetwork sends. |
| Probes | `arqma-storage`, `arqma-router`, `arqma-msg` | Separate processes. Daemon storage client URL defaults empty. `--arq-router` is off unless passed. |
| RPC | `get_arqnet_status`, `get_pulse_status`, `get_blink_status`, `get_storage_status` | Operator methods. Hidden on restricted RPC. |

Pulse acceptance (HF20 and HF21), as implemented:

1. Hard-fork major version must match the height table.
2. RandomARQ proof must pass.
3. If a Pulse extra is present, round, payload hash, and majority certificate must pass.
4. If a Pulse extra is absent, the block is still valid. The branch that would require a certificate also requires `pow_replacement_ready()`, which is hardcoded false.

## Claimed locks

| Claim | Result |
| --- | --- |
| Default `--arqnet-backend=legacy-arqnet` | Holds. `command_line_args.h` default string is `legacy-arqnet`. Unknown values fall back to legacy. |
| Mainnet `arqmq` needs `--arqnet-allow-experimental` | Holds. `resolve_backend` overrides mainnet `arqmq` to legacy unless the flag is set. Testnet and stagenet can select `arqmq` without that flag. |
| Pulse stage 2, PoW still required | Holds. `k_pulse_pow_stage = 2`. `pow_replacement_ready()` ignores that stage and returns false. `pow_required_for_block` returns true. Bumping the stage constant alone does not turn PoW off. |
| RandomARQ required at HF20 and HF21 | Holds. PoW checks in `Blockchain` stay gated by `pow_required_for_block`. |
| Quorum stays on SNNetwork unless HF20+ and CURVE `arqmq` | Holds. `native_mesh_live_at` needs HF ≥ 20, port stage ≥ 4, and a running stack with CURVE keys. Otherwise `relay_to_peers_impl` calls `snn.send`. |
| Mainnet `NETWORK_ID` unchanged | Holds. `cmake/GenNetworkId.cmake` keeps the historical UUID `11 11 11 11 FF FF FF 11 11 11 FF FF FF 11 11 1A`. Git commit and dirty-tree seeds apply only to testnet and stagenet. |
| HF19 mainnet height unchanged | Holds. `1886030` / time `1771099200`, same as upstream. |
| 628 unit tests | The count is stale. A local Release `unit_tests` run of this follow-up (same curated sources as the tip, no new tests) passed **636** tests. Ubuntu CI on `0d6db6b` succeeded; it does not publish the count in the check name. The figure is the `unit_tests` binary only (`BUILD_INTEGRATION_TESTS=OFF`), not core/functional/fuzz. |

## malbit's comments (still on the PR)

| Comment | On tip `0d6db6b` |
| --- | --- |
| `AUTO_VAL_INIT` was already removed upstream (`2d24362`) | The shim is gone from `contrib/epee` and from `src/`. Call sites remained in fuzz, core tests, functional tests, and `core_proxy`, so those targets would not compile. Replaced with value initialization in this follow-up. |
| Ports `22021` / `1090` in `docs/BINARIES.md`, `docs/OPERATOR_UPGRADE.md`, `arqma-msg`, `stack_env.hpp`, `arqma-storage`, `command_line_args.h`, `utils/arqma-stack.sh` | Code defaults are Arqma `19997` (storage) and `19998` (router). No `22021` or Lokinet `1090` defaults remain. The files malbit commented in `docs/` were deleted by `0d6db6b` ("docs cleanup"), so those review threads no longer point at live lines. |
| `burn_allowed_at_hf` should be `>` rather than `>=` | Do not change. Upstream block reward already uses `get_tx_miner_fee(tx, major_version >= network_version_19)` (`cryptonote_core.cpp` on master). The wallet rejects burns only when `hard_fork_version < 19`. Master's `construct_tx` check `<= 19` disagreed with both. `>= 19` matches validation and the wallet. Switching construction to `> 19` would refuse burns the chain already accounts at HF19. |

## Findings

### Critical

**`SocketStack` use-after-free on the 5s worker timeout.**  
`src/arqmq/socket_stack.cpp` (`send_to_peer`, `dispatch`, worker ctrl/jobs handlers).

`SendJob` / `Job` lived on the caller stack. The worker received a raw pointer and wrote `ec`, `done`, and the reply through it. `wait_for(..., 5s)` returned on timeout and destroyed that stack frame while `worker_send_to_peer` or `handle_job` could still be running. That is a heap/stack use-after-free on the experimental ArqMQ path (shadow sends and, after HF20, primary mesh sends). Default mainnet (`legacy-arqnet`) does not start the stack, so this does not fork today's chain. It is still memory corruption in a binary operators are invited to run with `--arqnet-backend=arqmq`.

**Fixed here.** The job state is a `shared_ptr` on the heap. The worker keeps its own reference, so a timeout no longer frees the object under the worker. Success path behavior (ping, ACL deny, peer send) is unchanged.

### High

**Cached block template restamps the timestamp and leaves a Pulse certificate bound to the old one.**  
`Blockchain::create_block_template` in `src/cryptonote_core/blockchain.cpp`.

On a cache hit the code set `m_btc.timestamp = time(NULL)` and returned the cached miner tx. `miner_payload_hash` includes that timestamp (`pulse.cpp`). `validate_miner_transaction` rejects the block when the extra is present and the hash does not match. After HF20, a miner that got a majority certificate and then asked for another template in a later second could mine a PoW-valid block that its own node rejects. Before HF20, `hybrid_sn_permitted` is false and there is no Pulse extra, so the historical cache stamp is what miners already do.

**Fixed here.** If the cached miner tx contains a Pulse extra and `time(NULL)` differs from the cached timestamp, the cache is discarded and the template is rebuilt. Same-second cache hits still work. Pre-HF20 templates are not invalidated by this check.

**Not fixed (intended policy, operator hazard).**  
`relay_to_peers_impl` in `src/cryptonote_protocol/arqnet.cpp`: when `native_mesh_live_at(hf)` is true, the send goes only through `primary_mesh_send_to_peer` and **skips** `snn.send`. `primary_mesh_send_to_peer` rewrites a non-empty hint to `port + 10000` (`k_mesh_shadow_port_offset` in `mesh_bridge.cpp`). Legacy peers listen on the normal Arq-Net port. A mixed HF20 fleet (some `--arqnet-backend=arqmq` with CURVE, some legacy) drops quorum, Pulse, and blink traffic from the native nodes onto a port the legacy nodes are not using.

This matches the lock "SNNetwork unless HF20+ and CURVE arqmq". It does **not** activate before height 4 000 000, and default mainnet never sets `native_mesh_live_at`. Changing it to dual-send would contradict that lock, so it is not patched here. Before any mainnet `arqmq` opt-in at HF20, either every quorum peer must be on the CURVE port or those nodes must stay on `legacy-arqnet`. HF21 does not add a second switch: exclusive outbound already happens at HF20 for nodes that have the stack. Nodes without a stack log a warning and keep SNNetwork.

### Medium

**Incomplete ZMQ multipart treated as a full command.**  
`recv_all_parts` in `src/arqmq/socket_stack.cpp` returned true when a later frame failed after `more()` was set. Callers then parsed a short frame list.

**Fixed here.** A short read now returns false and the message is dropped.

**Operator docs cited by the PR are not on the tip.**  
`0d6db6b` deletes `docs/` (including `PRODUCT.md`, `OPERATOR_UPGRADE.md`, `BINARIES.md`, and the OpenAPI stub). `README.md` still states the mainnet locks. `CHANGELOG-upgrade.md` still points at `docs/RELEASE_NOTES_v10.1.0.md`, which is gone. malbit's port notes were on the deleted guides; the code ports are already `19997` / `19998`. Restoring the whole tree would undo an explicit cleanup commit, so this follow-up does not put those files back. The PR description should be edited so it does not promise files the tip removed. Stagenet steps that lived only in `OPERATOR_UPGRADE.md` need a replacement before a public soak.

**Testnet hard-fork table gains HF19.**  
Upstream `master` testnet stops at `network_version_18` height 1100. This branch adds testnet v19 at 1200, v20 at 1300, v21 at 1400. A testnet chain that already passed 1200 as v18 will not follow this binary. Stagenet v19 at 220 is unchanged; v20/v21 are new (240 / 260). Mainnet v19 is unchanged. This is the published schedule, not an accidental mainnet fork. Confirm live testnet height before pointing public testnet at this binary.

**Testnet and stagenet `NETWORK_ID` are commit-scoped.**  
`GenNetworkId.cmake` hashes `arqma-network-id|<net>|<seed>`. A dirty tree uses `<commit>-dirty-<content hash>` and will not peer with a clean build of the same commit. Missing git uses seed `unknown`. Mainnet is not affected. Multi-machine stagenet requires every node to be built from the same clean commit. That is why "need stagenet tests" cannot be a mix of ad-hoc dirty builds.

### Low

**`peer_mesh_is_snnetwork()` is false whenever the SocketStack is up**, including before HF20 while live relay is still SNNetwork (`mesh_bridge.cpp`). `daemon.cpp` then warns "Peer mesh is not SNNetwork". The warning overstates the pre-HF20 case. Not changed.

**`checkpoints::is_alternative_block_allowed` returns true when `m_db` is null.** Production opens a database before this matters. The null check avoids crashes in unit tests. Hardcoded checkpoint rows are not removed relative to master.

**`get_n_service_nodes` with `limit == 0` returns the full list.** Same sentinel as upstream master (`if (req.limit != 0)`). Not a new amplification bug. Do not "fix" it to a cap or existing callers that pass 0 will change.

**Typo** `"epmty"` in an Arq-Net log line. Cosmetic.

### Speculative (not treated as defects)

- Alt-block admission runs `prevalidate_miner_transaction` and checks Pulse payload only when the block is promoted. Same shape as existing alt handling. A bogus Pulse extra on an alt tip is not a main-chain accept.
- `min_signatures_for_quorum` is `(n * 2) / 3` (7 of 11). `verify_majority_certificate` requires **exactly** that many signatures, which keeps the spliced extra weight stable. Seven-of-eleven is not a strict `> 2n/3` quorum. While RandomARQ still selects the block, two conflicting certificates do not choose the chain. Do not set `k_pulse_pow_stage` to 3 or make `pow_replacement_ready()` true without revisiting this threshold. Not changed.
- Inbound frames on an outbound DEALER are tagged ServiceNode from the peer table entry. Not shown to be reachable by an unauthenticated peer; CURVE is required for that socket.
- `staking_portions_to_percent` copies `STAKING_SHARE_PARTS` into a local before the cast to silence a Clang warning. The arithmetic is the same cast as before.

## What this follow-up changes

| File | Change |
| --- | --- |
| `src/arqmq/socket_stack.hpp`, `socket_stack.cpp` | Heap `shared_ptr` job state so the worker timeout cannot free caller stack. Incomplete multipart recv fails closed. |
| `src/cryptonote_core/blockchain.cpp` | Do not reuse a cached template whose Pulse extra is bound to a different timestamp. |
| `tests/fuzz/*`, `tests/core_tests/chaingen.*`, `tests/core_proxy/core_proxy.cpp`, `tests/functional_tests/transactions_flow_test.cpp` | Drop leftover `AUTO_VAL_INIT` now that the macro is gone. |

No edit to `hardfork.cpp`, `pulse.cpp` stage constants, `backend_policy.cpp`, or `NETWORK_ID`.

## Tests

Local build of this follow-up:

```text
cmake -S . -B build/upgrade-release -G Ninja \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_INTEGRATION_TESTS=OFF
cmake --build build/upgrade-release --parallel 4 --target unit_tests
./build/upgrade-release/tests/unit_tests/unit_tests
```

Result: `[  PASSED  ] 636 tests.` in 8.5s, exit 0. g++ 13, Release. The first configure attempt with `/usr/bin/c++` (Clang 18) failed at link (`-lstdc++`) and again at C++20 module scanning (`CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS-NOTFOUND`); the passing run used g++.

The PR text says 628. This tree's curated binary is 636. No test was added in the follow-up, so the 628 figure was already behind `0d6db6b`.

Upstream CI on `0d6db6b` (before these fixes): `ubuntu-24.04-unit-Release` and `ubuntu-24.04-unit-Debug` succeeded. `macos-14-unit-Release` failed in Homebrew, not in the test binary. `linux-asan-ubsan` was skipped.

Compiler noise, not a failure: `is_p2p_peer_version_acceptable` compares `version_build >= 0` on an unsigned field (`p2p_protocol_defs.h`). The minimum build is 0, so that last comparison is always true once major/minor/revision already matched `10.0.0`. Peers below 10.0.0.0 are still rejected. Left as-is.

## GitHub review comment (paste on arqma/arqma#377)

Reviewed tip `0d6db6b` against `arqma/arqma` master `4b067e8c`.

Mainnet locks hold. Default backend is `legacy-arqnet`. Mainnet `arqmq` without `--arqnet-allow-experimental` is forced back to legacy. Live quorum stays on SNNetwork until HF20 and a CURVE stack. `pow_replacement_ready()` is false and `pow_required_for_block` is true, so RandomARQ is still required at HF20 and HF21. Mainnet HF19 stays at height 1 886 030. Mainnet `NETWORK_ID` stays the historical UUID. Pulse extra is optional; if present it must match height, round, payload, and an exact majority certificate.

`BLOCKED` is review-required plus a red macOS job. That job failed in Homebrew before configure. Ubuntu unit Release and Debug on this tip succeeded. A local g++ Release run of the same curated binary passed 636 tests (the PR's "628" is behind the tip). The PR is mergeable as a git diff.

malbit: ports in code are `19997` / `19998` (the `docs/` threads are on files deleted in `0d6db6b`). `AUTO_VAL_INIT` is gone from epee; leftover call sites were only in non-unit tests. Please keep `burn_allowed_at_hf` as `>= 19`. Master already counts burns in the block fee with `major_version >= 19`, and the wallet allows HF19 burns. `> 19` would disagree with that.

Issues:

- Critical: `SocketStack::send_to_peer` / `dispatch` keep job state on the caller stack and return after 5s. The worker can write that freed stack. Default mainnet does not start this stack. Fixed on the follow-up branch by heap `shared_ptr` state (no policy change).
- High: cached block templates unconditionally restamp `timestamp` while a Pulse certificate binds the old timestamp, so an HF20 miner can produce a PoW-valid block that fails `validate_miner_transaction`. Fixed by rebuilding the template when the stamp changes. Pre-HF20 cache behavior is unchanged.
- High, not changed: at HF20, `arqmq`+CURVE nodes send quorum only on `port+10000` and skip SNNetwork. That matches the published lock, and it is a mixed-fleet outage if any SN opts in early. Stay on `legacy-arqnet` for mainnet until the whole quorum is on the new port.
- Medium: testnet gains HF19 at 1200 (master testnet has no v19 row). Stagenet/testnet network ids are per commit, and a dirty tree will not peer. Mainnet id is unchanged. `docs/PRODUCT.md` and `docs/OPERATOR_UPGRADE.md` are not on the tip; the PR body should stop citing them.

I would not merge until the SocketStack timeout and the Pulse template stamp are in the branch, and until a same-commit stagenet soak is actually run. Neither fix moves HF20, Pulse stage, or the default backend. Those two fixes, plus this write-up, are in https://github.com/ArqTras/arqma/pull/7.
