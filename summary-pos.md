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

## Functional behaviour overview

### Block wire format (Arqma Pulse slice)

After **`major_version >= network_version_20_pos`** the block exposes:

- **`pulse_header`** — `pulse_random_value` (16 bytes), `round`, `validator_bitset`; included in **`block_header`** and therefore in **`get_block_hash`** preimage via `block_header` cast during hashing.
- **`reward`** (`uint64_t`) — nominal total miner/coinbase-side amount for Pulse-era alignment checks.
- **`sn_winner_tail`** (`crypto::hash4`) — last four bytes of the declared service-node winner public key — sanity linkage with miner `tx_extra`; not the sole authority check.
- **`pulse_validator_signatures`** — capped list (`MAX_PULSE_VALIDATOR_SIGNATURES`); **excluded from block hash** (only in `block` tail, not in `block_header`).

Serialization: binary, Boost serialization, JSON (`serialization/json_object.*`).

### PoW suppression (when Pulse rules apply)

PoW hashing is **skipped** for incoming blocks only when **`FORK_ACTIVE`** and **`major_version >= network_version_20_pos`** (main chain and alternative chain paths in `Blockchain`).

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
- Block hash excludes validator signatures — correct preimage for validators.

 **`pulse_coinbase_matches_pulse_header`** (anonymous namespace helper in **`blockchain.cpp`**):

- Header **`reward`** must equal sum of **`miner_tx` outputs**.
- **`sn_winner_tail`** must equal last 4 bytes of **`get_service_node_winner_from_tx_extra(miner_tx.extra)`**.
- Used from **`validate_miner_transaction`** (main-chain path) and **`handle_alternative_block`** (alternative chain path so bad coinbase is rejected before resting as an alt).

### PoW block templates (< PoS-major version)

For **`major_version < network_version_20_pos`**, **`create_block_template`** clears **`pulse`**, **`reward`**, **`sn_winner_tail`**, and **`pulse_validator_signatures`** so reused `block` blobs cannot leak stale Pulse fields.

### Cumulative difficulty (Pulse tuning)

When **`FORK_ACTIVE`** and **`version >= network_version_20_pos`**, next block difficulty uses **`next_difficulty_pulse_pos`** (`difficulty.cpp`) — LWMA variant sharing the v16 core with target spacing **`DIFFICULTY_TARGET_V20_POS`** (60 s configured in `cryptonote_config.h`), instead of the pre-PoS **120 s** LWMA (`DIFFICULTY_TARGET_V16`). Applied on **main** and **alternate** difficulty paths.

### RPC / ZMQ telemetry

 **`get_info`** and **`DaemonInfo`** expose:

| Field | Role |
|--------|------|
| `pos_fork_active` | Mirrors `FORK_ACTIVE`. |
| `pos_planned_hf_name`, `pos_planned_fork_height` | Planned Arqma PoS rollout metadata. |
| `pos_target_block_time_sec`, `pos_quorum_validators_min`, `pos_signature_threshold` | Pulse tuning constants. |
| `pos_expects_sn_storage_server` | Operator hint: validators expect **`arqma-storage-server`** (ArqTras fork) paired with **`arqmad`** (e.g. `storage_server_ping`). |
| **`pos_pulse_blocks_since_fork`** | When `FORK_ACTIVE` and height exceeds planned fork rehearsal height — blocks elapsed since that rehearsal boundary (**informational**). |
| **`pos_pulse_next_round_wire_hint`** | `height % 256` suggestion for tooling / producer UX for **`pulse.round`** (**not enforced** by daemon consensus). |
| **`pos_pulse_cum_diff_uses_60s_lwma`** | **`true`** when **`FORK_ACTIVE`** and tip hard-fork major is **`>= network_version_20_pos`**, matching the branch that selects **`next_difficulty_pulse_pos`** for cumulative difficulty (restricted RPC returns **`false`**). |

The ordinary **`target`** difficulty field reflects **`Blockchain::get_difficulty_target()`** — already **`DIFFICULTY_TARGET_V20_POS`** under the same Pulse/LWMA gated conditions.

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

---

## Key paths (quick reference)

- `src/common/arqma_pulse_fork.h` — fork constants, rehearsal heights, quorum sizes, roadmap.
- `src/cryptonote_basic/cryptonote_basic.h` — Pulse structs and header fields.
- `src/cryptonote_basic/difficulty.cpp` / `.h` — LWMA cores; **`next_difficulty_pulse_pos`**.
- `src/cryptonote_core/blockchain.cpp` / `.h` — PoW skip, **`verify_pulse_fork_block_rules`**, Pulse coinbase/header checks, template sanitisation.
- `src/crypto/hash.h` — **`hash4`**, **`null_hash4`**.
- `src/rpc/core_rpc_server*.cpp/.h`, `daemon_handler.cpp`, `message_data_structs.h`, `serialization/json_object.cpp` — telemetry and JSON parity.
