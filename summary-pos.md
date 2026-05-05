# Arqma Planned PoS / Pulse — Change Summary

This document summarises Proof-of-Stake (**PoS**) and **Pulse**-style scaffolding on the **Arqma Network** codebase. Naming is intentionally **Arqma-centric** (no third-party branding in feature descriptions).

**Maintainers:** Append new bullets under **[Changelog (append-only)]** for each merged PoS/Pulse-related change — include UTC date (or sprint), commit hash, one-line summary, and a short functional note.

---

## Safety / activation model

| Item | Value |
|------|--------|
| **Runtime master switch** | `arqma::pulse_fork::FORK_ACTIVE` in `src/common/arqma_pulse_fork.h` — **must remain `false` in production until the network adopts PoS consensus.** |
| **Planned release label** | `HF_RELEASE_NAME` → `ARQMA-V11.0.0-PoS` |
| **Wire / consensus version marker** | `cryptonote::network_version_20_pos` (`cryptonote_config.h`) — hard-fork scheduling rows remain commented until go-live planning. |

When `FORK_ACTIVE` is **`false`** (default), most new validation paths below are **inactive**; RPC and types still expose PoS telemetry for tooling and rehearsals.

---

## Mainnet go-live checklist (enable full PoS / Pulse)

The items below are **engineering and operations steps** required before **mainnet** can safely run the full Pulse-style PoS stack as implemented in this tree. Treat this as a release runbook complement to `arqma_pulse_fork.h` constants and network governance decisions.

1. **Network decision** — Finalise block height (or voting window) and wall-clock cutoff for **`network_version_20_pos`** with the Arqma network; align **economic** rules at the fork boundary if they must change relative to HF16+. **Unless the network explicitly votes a different split**, governance / development / net **continue** through the **`construct_miner_tx`** path (**`gov` / `dev` / `net`** outputs keyed from **`get_config`** → **`GOV_WALLET_ADDRESS`**, **`DEV_WALLET_ADDRESS`**, **`NET_WALLET_ADDRESS`**); changing those strings or dropping the outputs requires **coordinated** updates to **`validate_miner_transaction`**, **`service_node_list::validate_miner_tx`**, and Pulse **`reward`** rules.
2. **Hard fork table** — In `src/cryptonote_basic/hardfork.cpp`, **uncomment / add** the mainnet row for **`cryptonote::network_version_20_pos`** with the chosen **height** and **timestamp** (`TBD_UNIX_TIME_MAINNET_POS` replaced with real values). Keep **stagenet / testnet** rows in sync for rehearsal.
3. **Fork constants** — In `src/common/arqma_pulse_fork.h`, set **`MAINNET_FORK_HEIGHT_PLANNED`** (and any RPC copy) to match the **same** activation height as the hard-fork row; reconcile **`PULSE_*`** quorum and timing constants with the final specification.
4. **Runtime activation** — For the **release binary** intended for mainnet PoS: set **`arqma::pulse_fork::FORK_ACTIVE = true`** (or introduce a **build flag** / single controlled define so release engineers cannot ship a mismatched toggle by accident). Until this is **`true`**, daemon consensus will **not** enforce Pulse signatures or PoW-skipping paths.
5. **Regtest / version pinning** — Remove or relax any **`cryptonote_core.cpp`** (or similar) **pinning** that forces the Ideal HF below **`network_version_20_pos`** for regtest-only workflows, if those networks must exercise v20.
6. **Service nodes** — Ensure a **minimum active SN set** and **checkpointing quorums** large enough that **`validators.size() >= PULSE_SIGNATURE_THRESHOLD`** can be satisfied on the canonical chain (and document decommission / testnet exemptions).
7. **Storage server pairing** — Require operators to run the **paired storage server** stack where **`POS_EXPECTS_STORAGE_SERVER_FOR_SERVICE_NODES`** applies; verify **`storage_server_ping`** / reachability policies match validator eligibility.
8. **Pulse producer / round network** — Extend **arqnet** (**rh** relay hops + **`core::copy_pulse_arqnet_vote_accumulator`**) with production policy: **`random_value`** agreement, merging accumulated votes into **`pulse_validator_signatures`** on the blob, **P2P** submission, and dedup / anti-spam beyond hop-limited gossip (in-tree: short **success-only** inbound dedup on **`pulse_proposal`** / **`pulse_vote`**; broader policy still TBD). Tooling **`create_next_pulse_block_template`** / **`get_pulse_block_template`** remains separate from acceptance.
9. **Wallets & RPC** — Confirm **wallet**, **mining**, and **RPC** surfaces refuse PoW where **`pow_mining_disabled_for_chain`** applies; extend any operator UIs to use **`pos_pulse_*`** telemetry and Pulse templates where needed.
10. **Rehearsal** — Run **stagenet** (and optional public testnet) with **`FORK_ACTIVE`** and real HF rows **before** mainnet; monitor **reorgs**, **alt-chain Pulse** blocks, **LWMA** (`next_difficulty_pulse_pos`), and **SNL hook** failures after `add_block`. In CI or locally, configure with **`BUILD_TESTS=ON`** and run **`unit_tests`** (includes **`PulseRound.*`** / **`PulseDifficulty.*`** in `tests/unit_tests/pulse_round.cpp`).
11. **Release & comms** — Publish **fork height**, **mandatory software version**, SN operator checklist, and rollback / emergency procedures; tag a **release** after final audit.

Until steps **2–4** and **8** are complete, mainnet **must not** be considered “full PoS live”; the daemon can **validate** Pulse-shaped blocks when enabled, but **block production** and **economic finality** depend on the transport and governance layer above.

---

## Functional behaviour overview

### Block wire format (Arqma Pulse slice)

After **`major_version >= network_version_20_pos`** the block exposes:

- **`pulse_header`** — `pulse_random_value` (16 bytes), `round`, `validator_bitset`; included in **`block_header`** and therefore in **`get_block_hash`** preimage via `block_header` cast during hashing.
- **`reward`** (`uint64_t`) — nominal total miner/coinbase-side amount for Pulse-era alignment checks.
- **`sn_winner_tail`** (`crypto::hash4`) — last four bytes of the declared service-node winner public key — sanity linkage with miner `tx_extra`; not the sole authority check.
- **`pulse_validator_signatures`** — capped list (`MAX_PULSE_VALIDATOR_SIGNATURES`); **excluded from block hash** (only in `block` tail, not in `block_header`).

Serialization: binary, Boost serialization, JSON (`serialization/json_object.*`).

### PoW suppression (when Pulse rules apply)

PoW hashing is **skipped** for incoming blocks only when **`FORK_ACTIVE`** and **`major_version >= network_version_20_pos`** (main chain and alternative chain paths in `Blockchain`). **Quorum signature verification** for those blocks is **not** done in that PoW branch: it runs from **`service_node_list::block_added`** / **`alt_block_added`** via **`Blockchain::verify_pulse_fork_block_rules`** (oxen-core style: after the block is accepted into the DB, the SNL hook can still abort and roll back).

Mining and block templates honour **`arqma::pulse_fork::pow_mining_disabled_for_chain()`**:

- **`major_version >= network_version_20_pos`** always disables PoW mining/templates for that template version **regardless** of `FORK_ACTIVE`.
- **`FORK_ACTIVE`** additionally disables PoW rehearsal at/after **`planned_fork_height_for_net()`** before the chain reaches PoS-major blocks.

Affected areas: **`blockchain.cpp`**, **`core_rpc_server.cpp`**, **`daemon_handler.cpp`**, **`miner.cpp`**, **`cryptonote_core.cpp`**.

### Consensus checks when `FORK_ACTIVE` (Pulse-era blocks)

 **`Blockchain::verify_pulse_fork_block_rules`** (`blockchain.cpp`):

- Requires **non-empty** `pulse_header` (no signature-only payloads).
- Requires **`pulse.random_value`** bytes not all-zero ( preimage binding for the Pulse round ).
- Requires at least **`PULSE_SIGNATURE_THRESHOLD`** signature entries.
- Requires at least **one** checkpointing quorum candidate (`main` or **alt** state) whose **`validators`** list has **`size() >= PULSE_SIGNATURE_THRESHOLD`** — otherwise no quorum can yield enough distinct valid votes.
- Verifies **`crypto::check_signature(block_hash, validator_pubkey, sig)`** against checkpointing quorum validators (`service_node_list::get_quorum(...)`) including **alternate-quorum candidates** where applicable.
- **`pulse.validator_bitset`**: bits may only reference validator indices present in the candidate quorum (`0` … `validators.size()-1`; at most **16** bits are representable). For **`voter_index < 16`**, a counted signature requires the corresponding bit to be set, and every set bit in that range must have a matching valid signature. Checkpointing quorums can have up to **`CHECKPOINT_QUORUM_SIZE` (20)** validators; indices **`16`–`19`** can still sign (no header bit) and count toward the threshold.
- Block hash excludes validator signatures — correct preimage for validators.

 **`pulse_coinbase_matches_pulse_header`** (anonymous namespace helper in **`blockchain.cpp`**):

- Header **`reward`** must equal sum of **`miner_tx` outputs**.
- **`sn_winner_tail`** must equal last 4 bytes of **`get_service_node_winner_from_tx_extra(miner_tx.extra)`**.
- Used from **`validate_miner_transaction`** (main-chain path) and **`handle_alternative_block`** (alternative chain path so bad coinbase is rejected before resting as an alt).

### Governance, dev, and net coinbase (PoS / Pulse)

**Requirement:** After the PoS/Pulse fork, **governance**, **development**, and **network** funds **continue** to use the same **`get_config(network, hf)` wallet strings** and the same **`construct_miner_tx`** layout as HF16+ PoW blocks:

- **`get_arqma_block_reward`** (`cryptonote_tx_utils.cpp`) sets **`gov`**, **`dev`**, **`net`** via **`dev_reward_formula`** for **`hard_fork_version >= 16`**, including **`network_version_20_pos`** (no separate zeroing branch).
- **`construct_miner_tx`** appends **three** deterministic outputs (governance, dev fund, net fund) after the miner output and service-node payout row(s), keyed from **`GOV_WALLET_ADDRESS`**, **`DEV_WALLET_ADDRESS`**, **`NET_WALLET_ADDRESS`** in **`cryptonote_config.h`** (mainnet constants include the published `ar2…` / `ar3…` addresses).
- **`Blockchain::validate_miner_transaction`** (**`hard_fork_version >= 16`**) still checks amounts and destination keys for the **last three** `miner_tx.vout` entries (gov / dev / net). **`Pulse` header `reward`** is the **full** coinbase total, so those slices are included in **`pulse_coinbase_matches_pulse_header`**.
- **Pulse templates** (**`create_next_pulse_block_template`** / **`core::get_pulse_block_template`**) build the coinbase through the same **`construct_miner_tx`** path, so allocations stay consistent as long as **`hard_fork_version`** on the block reflects the PoS-major version.

Changing vout order or dropping gov/dev/net for v20 would **break consensus** with existing validation; any future economic redesign must update **`construct_miner_tx`**, **`validate_miner_transaction`**, **`service_node_list::validate_miner_tx`**, and Pulse header rules **together**.

### PoW block templates (< PoS-major version)

For **`major_version < network_version_20_pos`**, **`create_block_template`** clears **`pulse`**, **`reward`**, **`sn_winner_tail`**, and **`pulse_validator_signatures`** so reused `block` blobs cannot leak stale Pulse fields.

### Pulse round timings & templates (oxen-parity scaffolding)

- **`src/cryptonote_core/pulse.h`** / **`pulse.cpp`** — **`get_round_timings`**, **`convert_time_to_round`**; constants **`PULSE_ROUND_TIMEOUT_SEC`**, **`PULSE_MAX_START_ADJUSTMENT_SEC`** in **`arqma_pulse_fork.h`**.
- **`Blockchain::create_next_pulse_block_template`** — Fills a main-chain Pulse template for a given **`service_nodes::block_winner`**, **`pulse.round`**, and **`validator_bitset`** (random value and signatures left for the round protocol). **`core::get_pulse_block_template`** forwards to it for integrators.

### Cumulative difficulty (Pulse tuning)

**`Blockchain::get_difficulty_for_next_block`** and **`get_next_difficulty_for_alternative_chain`** pick the LWMA branch from **`HardFork::get_ideal_version(height)`** for the **block being computed** (main: next chain height; alt: **`alt_block_height`**), not from the tip’s **current** fork index — so the **first** block after a version bump (e.g. first PoS height) already uses the PoS difficulty core when ideal HF is **`>= network_version_20_pos`**.

When **`FORK_ACTIVE`** and that ideal **`version >= network_version_20_pos`**, difficulty uses **`next_difficulty_pulse_pos`** (`difficulty.cpp`) — LWMA variant sharing the v16 core with target spacing **`DIFFICULTY_TARGET_V20_POS`** (60 s configured in `cryptonote_config.h`), instead of the pre-PoS **120 s** LWMA (`DIFFICULTY_TARGET_V16`).

### RPC / ZMQ telemetry

 **`get_info`** and **`DaemonInfo`** expose:

| Field | Role |
|--------|------|
| `pos_fork_active` | Mirrors `FORK_ACTIVE`. |
| `pos_planned_hf_name`, `pos_planned_fork_height` | Planned Arqma PoS rollout metadata. |
| `pos_target_block_time_sec`, `pos_quorum_validators_min`, `pos_signature_threshold` | Pulse tuning constants. |
| `pos_expects_sn_storage_server` | Operator hint: validators expect **`arqma-storage-server`** (ArqTras fork) paired with **`arqmad`** (e.g. `storage_server_ping`). |
| **`pos_pulse_blocks_since_fork`** | When `FORK_ACTIVE` and height exceeds planned fork rehearsal height — blocks elapsed since that rehearsal boundary (**informational**). |
| **`pos_pulse_next_round_wire_hint`** | When **`cryptonote::pulse::get_round_timings`** returns a value, **`cryptonote::pulse::convert_time_to_round(now, r0)`** (0–255); otherwise falls back to **`height % 256`**. **Not enforced** by consensus — operator UX only. |
| **`pos_pulse_cum_diff_uses_60s_lwma`** | **`true`** when **`FORK_ACTIVE`** and **ideal** HF for **`get_current_blockchain_height()`** (the next block’s height) is **`>= network_version_20_pos`**, aligned with **`get_difficulty_for_next_block`** (restricted RPC returns **`false`**). |

The ordinary **`target`** field is **`Blockchain::get_difficulty_target()`**: median spacing target seconds from **`get_current_diff_target`** applied to **ideal** HF for **`get_current_blockchain_height()`** (same height basis as cumulative-diff tuning), so **`DIFFICULTY_TARGET_V20_POS`** applies from the first PoS-major height onward, not one block late.

**Pulse block template RPC** — **`get_pulse_block_template`** / **`getpulseblocktemplate`** (`COMMAND_RPC_GET_PULSE_BLOCK_TEMPLATE`): JSON-RPC **`params`** include **`pulse_round`** (0–255), **`validator_bitset`** (≤65535), optional **`producer_pubkey`** (64 hex chars = service-node pubkey), optional **`pulse_random_value`** (32 hex chars = 16-byte header random — binds **`get_block_hash(b)`** before merge), and optional **`merge_arqnet_votes`** (**`true`**: copy **`pulse_validator_signatures`** from the in-process arqnet accumulator for that hash, capped at **`MAX_PULSE_VALIDATOR_SIGNATURES`**). Without **`producer_pubkey`**, the producer is **`get_block_winner()`**; with it, payouts are built for that **registered, active** SN via **`service_node_list::try_get_block_winner_for_service_node`**. Response mirrors **`getblocktemplate`**. Requires **`FORK_ACTIVE`** and ideal HF **`>= network_version_20_pos`**. Not forwarded to bootstrap daemon.

**Pulse arqnet vote buffer RPC** — **`get_pulse_arqnet_votes`** / **`getpulsearqnetvotes`** (`COMMAND_RPC_GET_PULSE_ARQNET_VOTES`): **`params.block_hash`** = 64 hex chars (`crypto::hash` of the **proposed** block). Response: **`found`**, **`chain_height`** (vote wire **`h`**), **`votes`** = `{ voter_index, signature hex }[]`, **`status`**. **`found`** is **`false`** when this process has no accumulator entry (arqnet off, wrong hash, or no votes yet). Requires **`FORK_ACTIVE`**; unsupported with bootstrap daemon (same pattern as **`get_pulse_block_template`**).

**Consensus (Pulse):** When **`FORK_ACTIVE`** and block HF is **`>= network_version_20_pos`**, **`service_node_list::validate_miner_tx`** accepts a coinbase SN winner that is **registered, active**, and appears on the **checkpointing quorum** (validators **or** workers) for **`height - 1`**, including **alt-quorum** candidates (same family as **`verify_pulse_fork_block_rules`**). Earlier HF still requires the scheduled **`get_block_winner()`** pubkey.

### arqnet Pulse wire (SN ↔ SN, ZMQ quorum channel)

Bindings live in **`src/arqnet/pulse_wire.h`**; handlers in **`src/cryptonote_protocol/arqnet.cpp`** — all **`SNNetwork::command_type::quorum`**. Accepted messages do **not** apply the block to the chain — use **P2P / `add_block`** for that. Responses use **`ok`** (1 = success), **`e`** error string when **`ok=0`**; **`bh`** where noted is raw **32-byte** `crypto::hash`.

**Relay (gossip):** optional **`rh`** = remaining relay hops (integer). If omitted, treated as **`DEFAULT_PULSE_RELAY_HOPS`** (**3**). After a **successful** **`pulse_proposal`** or **`pulse_vote`**, if **`rh` > 0** the daemon forwards the (possibly updated) bt-dict toward **checkpointing quorum** peers (same **`peer_info`** pattern as obligation vote relay: only if this SN is in the quorum subset and has peer targets). Forwarded payload uses **`rh − 1`**; at **`rh = 0`** no further hop is sent.

**Inbound dedup (success-only window):** after a **successful** accept, the daemon records **`bh`** (proposals) or **`cn_fast_hash(bh ‖ sig ‖ vi ‖ h)`** (votes) in an in-memory set with **~45 s** TTL and cap **4096** entries (LRU-ish eviction). A **later** duplicate within TTL gets **`ok=1`** + **`bh`** without re-running quorum / Pulse verify, **without** appending to the vote accumulator again, and **without** firing **relay** — so spam retries and mesh echo do not multiply work. **Rejected** messages are **not** entered (a failed proposal or bad vote can be retried immediately).

**Vote accumulator (per process):** each accepted **`pulse_vote`** **also** stores **`pulse_validator_signature_entry`** in an in-memory map keyed by **`bh`** (bounded, **64** block hashes, LRU by touch time). Read / clear via **`cryptonote::core::copy_pulse_arqnet_vote_accumulator`** / **`clear_pulse_arqnet_vote_accumulator`** (**`cryptonote_core.h`**) — backed by **`arqnet_pulse_vote_buffer_*`** function pointers set in **`init_core_callbacks`** (no-op if arqnet not initialized). After **`core::add_new_block`** succeeds for a block with **`major_version >= network_version_20_pos`**, the daemon calls **`clear_pulse_arqnet_vote_accumulator(get_block_hash(b))`** so accepted main- or alt-chain Pulse blocks do not leave stale quorum votes in RAM.

| Command | Payload (bt_dict parts) | Handler behaviour |
|---------|--------------------------|-------------------|
| **`pulse_cap`** | Optional **`!`** tag | Capability / fork telemetry (**`wv`**, **`fa`**, **`h`** = blockchain height, **`ihf`**, **`pmr`**). |
| **`pulse_proposal`** | **`blk`**, optional **`rh`** | Parse blob; when **`FORK_ACTIVE`** && block **`major_version >= network_version_20_pos`**, **`Blockchain::verify_pulse_fork_block_rules`**. If **`bh`** was **already accepted** within the dedup window, reply **`ok=1`** + **`bh`** only. Otherwise reply **`bh`** or **`ok=0`**; on success **`mark_seen(bh)`** then relay as above. |
| **`pulse_vote`** | **`bh`**, **`h`**, **`vi`**, **`sig`**, optional **`rh`** | Quorum / signature / sender binding; if the vote fingerprint was **already accepted** in-window, **`ok=1`** + **`bh`** only. Else append to accumulator, **`mark_seen`**, relay as above. |

The root **`summary-pos.md`** tracks this narrative; **`docs:`** commits that only touch `summary-pos.md` have no consensus impact.

### Wallets / mining UX

 **`simplewallet`** **status** and **`wallet_rpc_server`** **`start_mining`** forward daemon PoS refusal messages where applicable.

 After **`refresh`**, **`simplewallet`** prints optional **PoS rehearsal** block count, **`pos_pulse_next_round_wire_hint`**, and whether **`pos_pulse_cum_diff_uses_60s_lwma`** when PoS telemetry from **`get_info`** is present.

---

## Commit index (`c8aff143` … `HEAD` on PoS branch at time of writing)

Listed **oldest → newest**. Bodies abbreviated; refer to **`git show <hash>`** for full patches.

| Hash | Subject |
|------|---------|
| `c8aff143` | Scaffold planned PoS fork ARQMA-V11.0.0-PoS (inactive) — `arqma_pulse_fork.h`, `network_version_20_pos`, commented `hardfork.cpp` hooks, regtest pinning. |
| `9a28d644` | PoS scaffold: difficulty hook, `get_info` + ZMQ `DaemonInfo`, `planned_fork_height_for_net()`, `DIFFICULTY_TARGET_V20_POS`. |
| `3a100266` | PoS scaffold: difficulty block-window branch for HF20 scaffolding; roadmap/LWMA documentation notes. |
| `587ec52b` | PoS scaffold: optional PoW bypass on main and alt when `FORK_ACTIVE` + PoS-major. |
| `5e2699d2` | Disable PoW mining and templates in PoS scope; wallet/miner UX for refusal paths. |
| `a8a6c046` | Incremental Pulse: `pulse_header`, validator signatures, caps, serialization (binary/Boost). |
| `a9799ada` | SN + storage-server pairing docs/flags; initial Pulse-shape checks before full crypto verify. |
| `8f8adfc9` | Header fields: **`reward`**, **`sn_winner_tail`**, `crypto::hash4` (+ historical commit subject naming; implementation is **Arqma** Pulse-aligned). |
| `8e99bd42` | JSON tooling for Pulse block fields. |
| `b732afdf` | docs: prefer Arqma wording in Pulse comments. |
| `b0ead928` | Require non-empty Pulse header alongside quorum signatures; refresh roadmap/logs. |
| `36387220` | fix: declare `verify_pulse_fork_block_rules` on `Blockchain`. |
| `8adfe429` | Clear PoW template Pulse fields; share coinbase/header Pulse checks on alt ingestion. |
| `03ba201f` | `next_difficulty_pulse_pos` (60 s LWMA) + RPC `pos_pulse_*` telemetry. |
| `7f20161e` | `docs:` add **`summary-pos.md`** (living PoS/Pulse summary for maintainers). |
| `9dc3b9d2` | Reject zero **`pulse_random_value`** when verifying Pulse blocks; add **`pos_pulse_cum_diff_uses_60s_lwma`** to RPC / DaemonInfo / JSON. |
| `0ebf398c` | **`docs:`** refresh **`summary-pos.md`** after `9dc3b9d2`. |
| `dd8d4eaa` | Require **`validators.size() >= PULSE_SIGNATURE_THRESHOLD`** on some checkpointing quorum; extend **`simplewallet`** refresh PoS lines. |

*(If your branch diverged via history rewrite, re-run `git log --oneline <base>..HEAD` and reconcile this table.)*

---

## Changelog (append-only)

| Date | Commit | Summary | Functional notes |
|------|--------|---------|-------------------|
| 2026-05 | `c8aff143` … `03ba201f` | PoS/Pulse scaffolding series | PoW skip + quorum/crypto checks gated on `FORK_ACTIVE`; Pulse wire format; miner/RPC tooling; cum-diff LWMA pulse target 60 s; telemetry for fork offset and suggested `pulse.round`; Arqma SN + storage-server pairing expectation. |
| 2026-05-05 | `7f20161e` | Add `summary-pos.md` | Maintainer-facing English summary table + append-only changelog contract. |
| 2026-05-05 | `9dc3b9d2` | Pulse random preimage + LWMA-flag RPC | Consensus (when active): **`pulse_random_value`** must not be all-zero; operators see **`pos_pulse_cum_diff_uses_60s_lwma`** alongside existing PoS telemetry. |
| 2026-05-05 | `0ebf398c` | Update `summary-pos.md` | Document commit `9dc3b9d2` behaviour and extend commit index / changelog. |
| 2026-05-05 | `dd8d4eaa` | Quorum sizing + wallet PoS telemetry echo | Faster reject when no quorum is large enough; **`simplewallet`** **`refresh`** shows rehearsal depth, round modulus hint, 60 s LWMA flag. |
| 2026-05-05 | *(working tree)* | Pulse timings module + Pulse template + mainnet checklist doc | **`pulse.cpp`**: round timings / `convert_time_to_round`; **`create_next_pulse_block_template`**; SNL-deferred verify documented; RPC round hint from **`r0`** when timings available; **`summary-pos.md`**: English mainnet go-live checklist. |
| 2026-05-05 | *(working tree)* | Docs: gov/dev/net coinbase through PoS | Comments in **`cryptonote_config.h`**, **`cryptonote_tx_utils.cpp`**, **`blockchain.cpp`**, **`arqma_pulse_fork.h`**, **`summary-pos.md`**: Pulse keeps same HF16+ governance/dev/net vouts and mainnet **`ar*`** wallet constants unless consensus is changed as a bundle. |
| 2026-05-05 | *(working tree)* | RPC **`get_pulse_block_template`** | JSON-RPC **`get_pulse_block_template`** / **`getpulseblocktemplate`**; producer = current **`get_block_winner()`**; **`summary-pos.md`** updated. |
| 2026-05-05 | *(working tree)* | Optional **`producer_pubkey`** on Pulse template RPC | **`service_node_list::try_get_block_winner_for_service_node`**; shared payout builder with **`get_block_winner()`**; **`summary-pos.md`** consensus note for non-default producers. |
| 2026-05-05 | *(working tree)* | Pulse **`validate_miner_tx`** quorum producer | PoS-era: coinbase winner must be on checkpointing quorum at **`height-1`** (main + alt lists); **`summary-pos.md`** updated. |
| 2026-05-05 | *(working tree)* | **`validator_bitset`** vs signatures + ideal-HF difficulty | **`verify_pulse_fork_block_rules`**: bind 16-bit participation mask to votes (indices **≥16** bitless); **`get_difficulty_for_next_block`** / alt path + RPC **`pos_pulse_cum_diff_uses_60s_lwma`** use ideal HF for the target height. |
| 2026-05-05 | *(working tree)* | PoW template + RPC **`target`** align with next height | Main-chain **`create_block_template`**: **`major_version`** and timestamp window use **ideal** HF for **`height`** (matches cum-diff + RandomX seed gate on **`b.major_version`**); **`get_difficulty_target()`** uses ideal HF for **`get_current_blockchain_height()`**. |
| 2026-05-05 | *(working tree)* | Unit tests **`pulse_round.cpp`** | GTest: **`cryptonote::pulse::convert_time_to_round`** (0 / step / 255 / overflow) and **`next_difficulty_pulse_pos`** vs **`next_difficulty_v16`** on same window; **`BUILD_TESTS=ON`** in CMake. |
| 2026-05-05 | *(working tree)* | arqnet **`pulse_cap`** | **`pulse_wire.h`** + **`arqnet.cpp`**: SN-quorum command **`pulse_cap`** / reply with wire v1 + fork telemetry (**no consensus**); **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | arqnet **`pulse_proposal`** / **`pulse_vote`** | Serialized block sanity + optional **`verify_pulse_fork_block_rules`**; per-validator **`check_signature`** with quorum **`vi`** = sender pubkey; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | arqnet relay **`rh`** + vote accumulator | Quorum **`peer_info::relay_to_peers`** after success; **`core::copy_pulse_arqnet_vote_accumulator`** / **`clear_…`** + **`arqnet_pulse_vote_buffer_*`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | arqnet inbound Pulse dedup | **`arqnet.cpp`**: success-only **~45 s** / **4096** cap cache — **`bh`** for **`pulse_proposal`**, **`cn_fast_hash`** pack for **`pulse_vote`**; duplicates skip verify, accumulator double-append, and **`rh`** relay; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | RPC **`get_pulse_arqnet_votes`** | JSON-RPC read of **`core::copy_pulse_arqnet_vote_accumulator`** by **`block_hash`**; **`core_rpc_server_commands_defs.h`** / **`core_rpc_server.*`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | **`get_pulse_block_template`**: merge arqnet votes | Optional **`pulse_random_value`** + **`merge_arqnet_votes`** on **`COMMAND_RPC_GET_PULSE_BLOCK_TEMPLATE`**; **`core_rpc_server.cpp`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | Clear arqnet vote buffer on accepted Pulse block | **`core::add_new_block`**: after success, **`clear_pulse_arqnet_vote_accumulator`** for **`major_version >= network_version_20_pos`**; **`summary-pos.md`**. |

---

## Key paths (quick reference)

- `src/common/arqma_pulse_fork.h` — fork constants, rehearsal heights, quorum sizes, roadmap.
- `src/cryptonote_basic/cryptonote_basic.h` — Pulse structs and header fields.
- `src/cryptonote_basic/difficulty.cpp` / `.h` — LWMA cores; **`next_difficulty_pulse_pos`**.
- `src/cryptonote_core/blockchain.cpp` / `.h` — PoW skip, **`verify_pulse_fork_block_rules`**, Pulse coinbase/header checks, template sanitisation, **`create_next_pulse_block_template`**.
- `src/cryptonote_core/pulse.cpp` / `.h` — **`get_round_timings`**, **`convert_time_to_round`**.
- `src/cryptonote_core/cryptonote_core.cpp` / `.h` — **`get_pulse_block_template`**, **`copy_pulse_arqnet_vote_accumulator`**, **`clear_pulse_arqnet_vote_accumulator`**.
- `src/cryptonote_core/service_node_list.cpp` / `.h` — **`try_get_block_winner_for_service_node`**, **`validate_miner_tx`** (before **`network_version_20_pos`**: scheduled winner; PoS era: checkpointing-quorum producer at **`height-1`** + payout check).
- `src/crypto/hash.h` — **`hash4`**, **`null_hash4`**.
- `src/rpc/core_rpc_server*.cpp/.h`, `core_rpc_server_commands_defs.h`, `daemon_handler.cpp`, `message_data_structs.h`, `serialization/json_object.cpp` — telemetry, **`get_pulse_block_template`**, **`get_pulse_arqnet_votes`**, and JSON parity.
- `src/arqnet/pulse_wire.h`, `src/cryptonote_protocol/arqnet.cpp` — Pulse **arqnet** commands (**`pulse_cap`**, **`pulse_proposal`**, **`pulse_vote`**, **`rh`** relay, in-memory vote buffer).
