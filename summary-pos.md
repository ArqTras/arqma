# Arqma PoW/PoS Hybrid (Pulse) — Change Summary

This document summarises **PoW/PoS Hybrid** consensus (alternating heights: **PoW** and **Pulse**/service-node quorum slots) plus **Pulse** wire format and tooling on the **Arqma Network** codebase. Naming is **Arqma-centric** (no third-party branding in feature descriptions).

**Maintainers:** Append new bullets under **[Changelog (append-only)]** for each merged hybrid/Pulse-related change — include UTC date (or sprint), commit hash, one-line summary, and a short functional note. Short **Release / toolchain** notes that touch **`blockchain.cpp`**, **`arqnet.cpp`**, or **`arqma_pulse_fork.h`** belong in the same changelog.

---

## Safety / activation model

| Item | Value |
|------|--------|
| **Mainnet hybrid** | **`fork_active(MAINNET)`** follows the **baked-in HF table** (`mainnet_hard_forks[]` in `hardfork.cpp`): with the **`network_version_20` row commented** (default), hybrid is **off**; **uncomment the row** (+ timestamp) at go-live — no separate stagenet-only gate. Optional override: **`FORK_ACTIVE`** in `arqma_pulse_fork.h` (normally **`false`**). |
| **Stagenet** | Same mechanism: **`network_version_20`** is **present** in `stagenet_hard_forks[]`, so **`fork_active(STAGENET)`** is **true** in default builds. Pulse/PoW slot rules apply from **`planned_fork_height_for_net(STAGENET)`** when the chain is on HF **`>= network_version_20`**. **Testnet** has no v20 row until one is added. |
| **CMake `ARQMA_STAGENET_POS_REHEARSAL`** | **Legacy / optional.** Stagenet hybrid no longer depends on this flag; **`ON`** only adds compile define **`ARQMA_STAGENET_POS_REHEARSAL=1`** for downstream forks that still key off it. |
| **Regtest (`FAKECHAIN`, `--regtest`)** | **No compile-time harness:** **`fork_active(FAKECHAIN)`** is **`false`**; hybrid rules apply only where the HF schedule and tooling bring **`network_version_20`** onto the chain (extend **`regtest_hard_forks`** in **`cryptonote_core.cpp`** for local experiments, or use **stagenet**). |
| **Planned release label** | `HF_RELEASE_NAME` → **`ARQMA-V11.0.0-PoW-PoS-Hybrid`** |
| **Wire / consensus major version** | **`cryptonote::network_version_20`** in `cryptonote_config.h` — **mainnet** HF row for v20 stays **commented** until go-live; **stagenet** carries an active **`network_version_20`** row at **`STAGENET_FORK_HEIGHT_PLANNED`** (**`last_HF_before_v20 + 100`**, synced with **`STAGENET_LAST_HF_HEIGHT`** in `arqma_pulse_fork.h`). |

**`fork_active(net)`** — **`FORK_ACTIVE`** or **`HardFork::hard_fork_table_includes_pulse_hybrid(net)`** (any network whose hard-coded HF list includes **`>= network_version_20`**). Default **mainnet** stays pre-hybrid while the v20 row is **commented**; **stagenet** is hybrid-enabled via its active v20 row.

---

## PoW/PoS Hybrid — rules of operation (English)

This section states how the hybrid layer behaves **in this codebase** once the canonical chain reaches **`major_version >= network_version_20`** **and** **`fork_active(net)`**. Source of truth: **`src/common/arqma_pulse_fork.h`** (`planned_fork_height_for_net`, `is_pow_slot_at_height`, `is_pulse_slot_at_height`, `pow_mining_disabled_for_chain`) plus **`blockchain.cpp`** (validation, templates, cumulative difficulty branches).

### 1. Planned fork height and alternation by height

Let **`fh = planned_fork_height_for_net(net)`** (mainnet placeholder constant; stagenet from HF table + constants; **`FAKECHAIN`** uses the **`default`** branch unless you align schedules elsewhere). From **`fh`** onward **and** while the block’s **`major_version >= network_version_20`**:

| Slot type | Condition on block height **`h`** | Expected block shape |
|-----------|--------------------------------------|----------------------|
| **PoW slot** | `(h − fh) % 2 == 0` | Classic RandomX PoW checks apply; **`pulse_header`**, **`reward`**, **`sn_winner_tail`**, **`pulse_validator_signatures`** must be empty for that block (**PoW-shaped** template). First block **`h == fh`** is a **PoW** slot by this rule. |
| **Pulse slot** | `(h − fh) % 2 == 1` | Proof-of-work **hash check is skipped** (PoW not used); block must satisfy **Pulse** rules: non-empty Pulse header preimage binding, quorum signatures (**`verify_pulse_fork_block_rules`**, **`PULSE_SIGNATURE_THRESHOLD`**), **`pulse_coinbase_matches_pulse_header`**, SN winner rules (**`service_node_list`**) compatible with Pulse production. |

If **`h < fh`** or **`major_version < network_version_20`**, hybrid alternation **does not** apply; legacy PoW-era rules remain.

### 2. Mining and RPC templates

- **`pow_mining_disabled_for_chain(net, ideal_major_next, height_next)`**: after **`fh`**, **`ideal_major_next >= network_version_20`** — PoW templates and mining **are allowed only on PoW slots**; on **Pulse** slots they are **disabled** (use **`get_pulse_block_template`** / Pulse tooling). Before **`fh`** **and** `fork_active` with pre-v20 major version, legacy behaviour can keep PoW disabled for the “rehearsal below v20” path (see **`pow_mining_disabled_for_chain`** implementation).
- **`get_pulse_block_template`**: refused when **`!fork_active(net)`**, or when **next height is a PoW slot** — operators must use **`getblocktemplate` / PoW mining** for that height.
- Pulse **arqnet** paths use **`fork_active(core nettype)`** (HF table **or** **`FORK_ACTIVE`**) so **mainnet after go-live** and **stagenet** share the same gating semantics.

### 3. Difficulty target selection (high level)

- **PoW slot** (hybrid era): LWMA **`next_difficulty_v16`-style core** scaled by **`scale_pow_difficulty_for_hybrid_pow_slot`** (**active service-node count** factor relative to **`POW_DIFFICULTY_ACTIVE_SN_REFERENCE`**).
- **Pulse slot**: **`next_difficulty_pulse_pos`** (**60 s** LWMA **`DIFFICULTY_TARGET_V20_POS`**) aligned with Pulse slot pacing.
- **Ideal HF** at the **height being computed** (not lagging fork index by one) selects the branch so the **first v20-height** picks the intended rule-set immediately.

### 4. Mainnet vs stagenet

- **Stagenet** today: **HF `network_version_20` in the baked-in table** → **`fork_active(STAGENET)`** is **true**; hybrid alternation applies from **`fh`** when the chain is on v20 rules.
- **Mainnet**: **default binaries** ship with the **`network_version_20` row commented** (`hardfork.cpp`) → **`fork_active(MAINNET)`** is **`false`**; **uncomment the row** (and **`MAINNET_FORK_HEIGHT_PLANNED`**) at go-live. Optionally set **`FORK_ACTIVE`** only if you need a build-time override independent of the table.

### 5. Operator summary

Hybrid is **not** “PoW off forever”: it is **strict alternation**. Miners/SN tooling must distinguish **parity of `(height − fh)`** — odd → Pulse production path; even → PoW path. Service nodes still pair **arqma-storage-server** where **`POS_EXPECTS_STORAGE_SERVER_FOR_SERVICE_NODES`** applies (**Pulse-slot** quorum UX).

---

## Mainnet go-live checklist (enable PoW/PoS Hybrid on mainnet)

The items below are **engineering and operations steps** before **mainnet** can run **PoW/PoS Hybrid** (alternating PoW and Pulse slots) as implemented in this tree. Stagenet already exercises much of this path with default builds.

1. **Network decision** — Finalise block height (or voting window) and wall-clock cutoff for **`network_version_20`** with the Arqma network; align **economic** rules at the fork boundary if they must change relative to HF16+. **Unless the network explicitly votes a different split**, governance / development / net **continue** through **`construct_miner_tx`** (**`gov` / `dev` / `net`** from **`get_config`** → **`GOV_WALLET_ADDRESS`**, **`DEV_WALLET_ADDRESS`**, **`NET_WALLET_ADDRESS`**); changing those strings or dropping outputs requires **coordinated** updates to **`validate_miner_transaction`**, **`service_node_list::validate_miner_tx`**, and Pulse **`reward`** rules.
2. **Hard fork table** — In `src/cryptonote_basic/hardfork.cpp`, **uncomment / add** the mainnet row for **`cryptonote::network_version_20`** with the chosen **height** and **timestamp**. Keep **testnet / public rehearsal nets** aligned with governance.
3. **Fork constants** — In `src/common/arqma_pulse_fork.h`, set **`MAINNET_FORK_HEIGHT_PLANNED`** (and checkpoints if any) to match the **same** activation height as the HF row; reconcile **`PULSE_*`**, **`POW_DIFFICULTY_ACTIVE_SN_REFERENCE`**, and timing constants with the final specification.
4. **Runtime activation** — **`fork_active(MAINNET)`** is **`FORK_ACTIVE || HardFork::hard_fork_table_includes_pulse_hybrid(MAINNET)`**. Ship **`FORK_ACTIVE = false`**; **enable hybrid on mainnet by uncommenting the v20 HF row** (together with checklist items 2–3). Use **`FORK_ACTIVE = true`** only as an intentional override beyond the HF table if your release process requires it.
5. **Regtest / version pinning** — If local **`FAKECHAIN`** tests need **`network_version_20`**, extend **`regtest_hard_forks`** in **`cryptonote_core.cpp`** deliberately; otherwise regtest stays pre-v20 by default (**`fork_active(FAKECHAIN)`** is **`false`**). Prefer **stagenet** for end-to-end hybrid rehearsal.
6. **Service nodes** — Ensure active SN counts and checkpointing quorums can meet **`PULSE_SIGNATURE_THRESHOLD`** on Pulse slots (**odd** **`(height − fh)`** after **`fh`**); document decommission exemptions.
7. **Storage server pairing** — Operators run **arqma-storage-server** where **`POS_EXPECTS_STORAGE_SERVER_FOR_SERVICE_NODES`** applies; **`storage_server_ping`** policy matches eligibility.
8. **Pulse producer / round network** — Production **arqnet** policy (**`pulse_proposal`**, **`pulse_vote`**, **`rh`** relay, vote accumulator **`copy` / `clear`**, merge into **`get_pulse_block_template`**) validated under load; see **[PoW/PoS Hybrid — rules of operation](#powpos-hybrid--rules-of-operation-english)**.
9. **Wallets & RPC** — Mining uses PoW templates **only on PoW slots**; Pulse slots use Pulse RPC. Telemetry: **`pos_pulse_*`** and **`arqmad`/`simplewallet`** hybrid strings — see **[Hybrid integration audit (code surfaces)](#hybrid-integration-audit-code-surfaces)**.
10. **Rehearsal** — **Stagenet** (`--stagenet`): default binaries already include **`network_version_20`** and **`fork_active`**. Optionally keep **`-DARQMA_STAGENET_POS_REHEARSAL=ON`** only for legacy compile markers. Monitor **reorgs**, **alt-chain Pulse** blocks, **LWMA branches** (**PoW-slot scale** vs **`next_difficulty_pulse_pos`**), **SNL** hook failures. CI: **`BUILD_TESTS=ON`** and any Pulse/unit coverage you maintain in-tree.
11. **Release & comms** — Publish **`fh`**, parity rule (**PoW vs Pulse** height schedule), mandatory version, SN checklist, rollback procedures.

Until steps **2–4** and **8** align on mainnet, do not treat mainnet as **hybrid-live** even if the code supports it elsewhere.

**Technical “full path” in this tree (stagenet / rehearsal):** SN **arqnet** gossip fills the vote accumulator → **`get_pulse_block_template`** with **`merge_arqnet_votes`** → **`submit_block`** ( **`core::handle_block_found`** → DB + **P2P** relay). The HTTP script **`pulse_http_rehearsal.py`** implements **poll-and-submit** (**`--watch-submit`**) and **single-shot submit** (**`--submit-if-ready`**). That is **end-to-end block production over RPC** given live quorum traffic; it is **not** a substitute for mainnet checklist items (HF activation, economics, mandatory release).

---

## Functional behaviour overview

### Block wire format (Arqma Pulse slice)

After **`major_version >= network_version_20`** the block exposes:

- **`pulse_header`** — `pulse_random_value` (16 bytes), `round`, `validator_bitset`; included in **`block_header`** and therefore in **`get_block_hash`** preimage via `block_header` cast during hashing.
- **`reward`** (`uint64_t`) — nominal total miner/coinbase-side amount for **Pulse-slot** alignment checks (**PoW** slots keep **`reward` cleared** in template sanitisation paths).
- **`sn_winner_tail`** (`crypto::hash4`) — last four bytes of the declared service-node winner public key — sanity linkage with miner `tx_extra`; not the sole authority check.
- **`pulse_validator_signatures`** — capped list (`MAX_PULSE_VALIDATOR_SIGNATURES`); **excluded from block hash** (only in `block` tail, not in `block_header`).

Serialization: binary, Boost serialization, JSON (`serialization/json_object.*`).

### PoW vs Pulse validation (hybrid)

**Incoming blocks** at **`fork_active(net)`**, **`major_version >= network_version_20`**, **`height ≥ fh`**:

- **PoW slots** (**`is_pow_slot_at_height`**) — **`check_hash`** / RandomX PoW enforced; Pulse fields cleared in template path; **`pulse_coinbase_matches_pulse_header`** accepts empty Pulse (**PoW-shaped** header).
- **Pulse slots** (**`is_pulse_slot_at_height`**) — **PoW hash check skipped**; **`verify_pulse_fork_block_rules`** (+ coinbase/header checks) run from **`service_node_list::block_added`** / **`alt_block_added`** after DB accept (**SNL hook** — can still roll back).

**Mining / templates**: **`pow_mining_disabled_for_chain`** blocks PoW on **Pulse** slots (and retains the pre-v20 “rehearsal” path **`≥ fh`** when major is still **< v20** per implementation). **`major_version < network_version_20`** clears Pulse fields so blobs do not leak stale Pulse data.

Affected areas: **`blockchain.cpp`**, **`core_rpc_server.cpp`**, **`daemon_handler.cpp`**, **`miner.cpp`**, **`cryptonote_core.cpp`**.

### Consensus checks when `fork_active(net)` (**Pulse-slot** blocks)

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

### Governance, dev, and net coinbase (hybrid Pulse slots)

**Requirement:** Through **`network_version_20`** (including **Pulse** slots), **governance**, **development**, and **network** funds **continue** using the same **`get_config(network, hf)` wallet strings** and **`construct_miner_tx`** layout as HF16+ PoW blocks:

- **`get_arqma_block_reward`** (`cryptonote_tx_utils.cpp`) sets **`gov`**, **`dev`**, **`net`** via **`dev_reward_formula`** for **`hard_fork_version >= 16`**, including **`network_version_20`** (no separate zeroing branch).
- **`construct_miner_tx`** appends **three** deterministic outputs (governance, dev fund, net fund) after the miner output and service-node payout row(s), keyed from **`GOV_WALLET_ADDRESS`**, **`DEV_WALLET_ADDRESS`**, **`NET_WALLET_ADDRESS`** in **`cryptonote_config.h`** (mainnet constants include the published `ar2…` / `ar3…` addresses).
- **`Blockchain::validate_miner_transaction`** (**`hard_fork_version >= 16`**) still checks amounts and destination keys for the **last three** `miner_tx.vout` entries (gov / dev / net). **`Pulse` header `reward`** is the **full** coinbase total, so those slices are included in **`pulse_coinbase_matches_pulse_header`**.
- **Pulse templates** (**`create_next_pulse_block_template`** / **`core::get_pulse_block_template`**) build the coinbase through the same **`construct_miner_tx`** path, so allocations stay consistent as long as **`hard_fork_version`** on the block reflects **`network_version_20`** where applicable.

Changing vout order or dropping gov/dev/net for v20 would **break consensus** with existing validation; any future economic redesign must update **`construct_miner_tx`**, **`validate_miner_transaction`**, **`service_node_list::validate_miner_tx`**, and Pulse header rules **together**.

### PoW block templates (`< network_version_20`)

For **`major_version < network_version_20`**, **`create_block_template`** clears **`pulse`**, **`reward`**, **`sn_winner_tail`**, and **`pulse_validator_signatures`** so reused `block` blobs cannot leak stale Pulse fields.

### Pulse round timings & templates (oxen-parity scaffolding)

- **`src/cryptonote_core/pulse.h`** / **`pulse.cpp`** — **`get_round_timings`**, **`convert_time_to_round`**; constants **`PULSE_ROUND_TIMEOUT_SEC`**, **`PULSE_MAX_START_ADJUSTMENT_SEC`** in **`arqma_pulse_fork.h`**.
- **`Blockchain::create_next_pulse_block_template`** — Fills a main-chain Pulse template for a given **`service_nodes::block_winner`**, **`pulse.round`**, and **`validator_bitset`** (random value and signatures left for the round protocol). **`core::get_pulse_block_template`** forwards to it for integrators.

### Cumulative difficulty (Pulse tuning)

**`Blockchain::get_difficulty_for_next_block`** and **`get_next_difficulty_for_alternative_chain`** use **`HardFork::get_ideal_version(height)`** for the **height being computed**. At **`ideal >= network_version_20`** and **`fork_active`**, the chosen branch tracks **PoW-slot** vs **Pulse-slot** (**scaled v16-style PoW** vs **`next_difficulty_pulse_pos`**, 60 s **`DIFFICULTY_TARGET_V20_POS`**) so the correct target applies from the **first** v20 height without one-block skew.

### RPC / ZMQ telemetry

 **`get_info`** and **`DaemonInfo`** expose:

| Field | Role |
|--------|------|
| `pos_fork_active` | Mirrors **`fork_active(nettype)`** — **`FORK_ACTIVE`** or HF table includes **`network_version_20`** for that nettype. |
| `pos_planned_hf_name`, `pos_planned_fork_height` | Planned **`HF_RELEASE_NAME`** and **`planned_fork_height_for_net`**. |
| `pos_target_block_time_sec`, `pos_quorum_validators_min`, `pos_signature_threshold` | Pulse tuning constants. |
| `pos_expects_sn_storage_server` | Operator hint: validators expect **`arqma-storage-server`** (ArqTras fork) paired with **`arqmad`** (e.g. `storage_server_ping`). |
| **`pos_pulse_blocks_since_fork`** | When **`fork_active`** and **`height > fh`** — **`height − fh`** (informational rehearsal depth). |
| **`pos_pulse_next_round_wire_hint`** | When **`cryptonote::pulse::get_round_timings`** returns a value, **`cryptonote::pulse::convert_time_to_round(now, r0)`** (0–255); otherwise falls back to **`height % 256`**. **Not enforced** by consensus — operator UX only. |
| **`pos_pulse_cum_diff_uses_60s_lwma`** | **`true`** when **`fork_active(net)`** and **ideal** HF for the **next** block height is **`>= network_version_20`** (flags the Pulse-difficulty branch when it applies; restricted RPC clears to **`false`**). |
| **`pos_pulse_arqnet_vote_buffer_blocks`** | Distinct proposed-block hashes (**`bh`**) in the in-process arqnet vote accumulator (0–**64**); **`0`** on restricted **`get_info`** or when arqnet is not wired. **`core::get_pulse_arqnet_vote_buffer_distinct_block_count`**. |

The ordinary **`target`** field is **`Blockchain::get_difficulty_target()`**: median spacing target seconds from **`get_current_diff_target`** applied to **ideal** HF for **`get_current_blockchain_height()`** (same height basis as cumulative-diff tuning), so hybrid-era targets apply **from the first v20-height** without one-block skew.

**Pulse block template RPC** — **`get_pulse_block_template`** / **`getpulseblocktemplate`** (`COMMAND_RPC_GET_PULSE_BLOCK_TEMPLATE`): JSON-RPC **`params`** include **`pulse_round`** (0–255), **`validator_bitset`** (≤65535), optional **`producer_pubkey`** (64 hex chars = service-node pubkey), optional **`pulse_random_value`** (32 hex chars = 16-byte header random — binds **`get_block_hash(b)`** before merge), and optional **`merge_arqnet_votes`** (**`true`**: copy **`pulse_validator_signatures`** from the in-process arqnet accumulator for that hash, capped at **`MAX_PULSE_VALIDATOR_SIGNATURES`**, skipping votes with **`voter_index` < 16** unless the corresponding bit is set in **`validator_bitset`** — aligns with **`verify_pulse_fork_block_rules`** header-bit expectations). Without **`producer_pubkey`**, the producer is **`get_block_winner()`**; with it, payouts are built for that **registered, active** SN via **`service_node_list::try_get_block_winner_for_service_node`**. Response mirrors **`getblocktemplate`** plus **`pulse_template_block_hash`** (hex **`get_block_hash(b)`** for the returned blob — same id **`get_pulse_arqnet_votes`** / arqnet votes use), **`merged_arqnet_vote_count`**, **`pulse_signature_threshold`** (consensus floor), and **`merged_pulse_signatures_meet_threshold`** (`merged_arqnet_vote_count >= threshold`). Requires **`fork_active(net)`**, next chain height **Pulse slot** (**`is_pulse_slot_at_height`**), ideal HF **`>= network_version_20`**. Not forwarded to bootstrap daemon.

**Pulse arqnet vote buffer RPC** — **`get_pulse_arqnet_votes`** / **`getpulsearqnetvotes`** (`COMMAND_RPC_GET_PULSE_ARQNET_VOTES`): **`params.block_hash`** = 64 hex chars (`crypto::hash` of the **proposed** block). Response: **`found`**, **`chain_height`** (vote wire **`h`**), **`votes`** = `{ voter_index, signature hex }[]`, **`status`**. **`found`** is **`false`** when this process has no accumulator entry (arqnet off, wrong hash, or no votes yet). Requires **`fork_active(net)`**; unsupported with bootstrap daemon (same pattern as **`get_pulse_block_template`**).

**Consensus (Pulse slot):** When **`fork_active(net)`**, block HF **`>= network_version_20`**, and the block sits on a **Pulse** height (odd **`(height − fh)`** in hybrid scheduling), **`service_node_list::validate_miner_tx`** accepts a coinbase SN winner that is **registered, active**, and appears on the **checkpointing quorum** (validators **or** workers) for **`height - 1`**, including **alt-quorum** candidates (same family as **`verify_pulse_fork_block_rules`**). Earlier HF still requires the scheduled **`get_block_winner()`** pubkey.

### arqnet Pulse wire (SN ↔ SN, ZMQ quorum channel)

Bindings live in **`src/arqnet/pulse_wire.h`**; handlers in **`src/cryptonote_protocol/arqnet.cpp`** — all **`SNNetwork::command_type::quorum`**. Accepted messages do **not** apply the block to the chain — use **P2P / `add_block`** for that. Responses use **`ok`** (1 = success), **`e`** error string when **`ok=0`**; **`bh`** where noted is raw **32-byte** `crypto::hash`.

**Relay (gossip):** optional **`rh`** = remaining relay hops (integer). If omitted, treated as **`DEFAULT_PULSE_RELAY_HOPS`** (**3**). After a **successful** **`pulse_proposal`** or **`pulse_vote`**, if **`rh` > 0** the daemon forwards the (possibly updated) bt-dict toward **checkpointing quorum** peers (same **`peer_info`** pattern as obligation vote relay: only if this SN is in the quorum subset and has peer targets). Forwarded payload uses **`rh − 1`**; at **`rh = 0`** no further hop is sent.

**Inbound dedup (success-only window):** after a **successful** accept, the daemon records **`bh`** (proposals) or **`cn_fast_hash(bh ‖ sig ‖ vi ‖ h)`** (votes) in an in-memory set with **~45 s** TTL and cap **4096** entries (LRU-ish eviction). A **later** duplicate within TTL gets **`ok=1`** + **`bh`** without re-running quorum / Pulse verify, **without** appending to the vote accumulator again, and **without** firing **relay** — so spam retries and mesh echo do not multiply work. **Rejected** messages are **not** entered (a failed proposal or bad vote can be retried immediately). When **`clear_pulse_arqnet_vote_accumulator(bh)`** runs (same path as a successfully accepted **`network_version_20`** Pulse-slot block on this process), the **proposal** fingerprint keyed by **`bh`** is also removed from that window so the same block id is not held as a faux-duplicate for the TTL.

**Vote accumulator (per process):** each accepted **`pulse_vote`** **also** stores **`pulse_validator_signature_entry`** in an in-memory map keyed by **`bh`** (bounded, **64** block hashes, LRU by touch time). Read / clear via **`cryptonote::core::copy_pulse_arqnet_vote_accumulator`** / **`clear_pulse_arqnet_vote_accumulator`** (**`cryptonote_core.h`**) — backed by **`arqnet_pulse_vote_buffer_*`** function pointers set in **`init_core_callbacks`** (no-op if arqnet not initialized). After **`core::add_new_block`** succeeds for a block with **`major_version >= network_version_20`**, the daemon calls **`clear_pulse_arqnet_vote_accumulator(get_block_hash(b))`** so accepted main- or alt-chain Pulse blocks do not leave stale quorum votes in RAM.

| Command | Payload (bt_dict parts) | Handler behaviour |
|---------|--------------------------|-------------------|
| **`pulse_cap`** | Optional **`!`** tag | Capability / fork telemetry (**`wv`**, **`fa`**, **`h`** = blockchain height, **`ihf`**, **`pmr`**). |
| **`pulse_proposal`** | **`blk`**, optional **`rh`** | Per-sender rate limit: combined **`pulse_proposal`** + **`pulse_vote`** capped by **`arqma::pulse_wire::PULSE_QUORUM_PEER_RATE_*`** (hex sender key); **`ok=0`** + **`e`** `pulse proposal rate limit` when exceeded. Then: parse blob; when **`fork_active(net)`** && block **`major_version >= network_version_20`**, **`Blockchain::verify_pulse_fork_block_rules`**. If **`bh`** was **already accepted** within the dedup window, reply **`ok=1`** + **`bh`** only. Otherwise reply **`bh`** or **`ok=0`**; on success **`mark_seen(bh)`** then relay as above. |
| **`pulse_vote`** | **`bh`**, **`h`**, **`vi`**, **`sig`**, optional **`rh`** | Same per-sender cap as **`pulse_proposal`**; **`ok=0`** + **`e`** `pulse vote rate limit` when exceeded. Quorum / signature / sender binding; if the vote fingerprint was **already accepted** in-window, **`ok=1`** + **`bh`** only. Else append to accumulator, **`mark_seen`**, relay as above. |

The root **`summary-pos.md`** tracks this narrative; **`docs:`** commits that only touch `summary-pos.md` have no consensus impact.

### Wallets / mining UX

 **`simplewallet`** **`start_mining`** and **`wallet_rpc_server`** **`start_mining`** proxy the daemon **`/start_mining`** endpoint; on **Pulse** slots (hybrid **`pow_mining_disabled_for_chain`**) the daemon returns **`POW_MINING_DISABLED_RPC_MESSAGE`** — surfaced as the wallet error string.

 After **`status`** or a successful **`refresh`**, **`simplewallet`** calls **`get_info`** (via **`maybe_print_daemon_pos_info`**) and, when **`pos_fork_active`** or **`pos_planned_fork_height > 0`**, prints **PoW/PoS Hybrid** metadata (planned HF label, **`fh`**, storage-server expectation, rehearsal depth, arqnet buffer, round hint, 60 s LWMA flag).

---

## Hybrid integration audit (code surfaces)

Audit of **in-tree** surfaces for **hybrid PoW refusal on Pulse slots**, **Pulse templates / votes**, and **`pos_*` / `pos_pulse_*` telemetry**. “**OK**” means wired for intended behaviour; “**Gap**” means UX/API polish (not necessarily a consensus bug).

| Surface | Location | Behaviour | Status |
|---------|----------|-----------|--------|
| PoW block template | **`Blockchain::create_block_template`** (`blockchain.cpp`) | Returns **`false`** when **`pow_mining_disabled_for_chain`** — PoW templates stop at the chain layer. | OK |
| **`getblocktemplate`** | **`core_rpc_server::on_getblocktemplate`** | Before template build: **`pow_mining_disabled_for_chain`** → **`error_resp.code = CORE_RPC_ERROR_CODE_POW_MINING_DISABLED` (-14)** and **`error_resp.message = POW_MINING_DISABLED_RPC_MESSAGE`**. Other failures remain generic internal error. | OK |
| **`start_mining` (HTTP)** | **`core_rpc_server::on_start_mining`** | **`pow_mining_disabled_for_chain`** → **`res.status = POW_MINING_DISABLED_RPC_MESSAGE`** (not **`CORE_RPC_STATUS_OK`**). | OK |
| **`start_mining` (ZMQ)** | **`daemon_handler::handle(StartMining)`** | Same check; **`error_details`** carries **`POW_MINING_DISABLED_RPC_MESSAGE`**. | OK |
| **`get_info` (HTTP)** | **`core_rpc_server::on_get_info`** | Full **`pos_*` / `pos_pulse_*`** when **not** restricted; restricted mode clears planned-fork constants and **`pos_pulse_*`** hints **except** **`pos_fork_active`** still reflects **`fork_active(nettype)`** (e.g. **true on stagenet**). | OK |
| **`get_info` / DaemonInfo (ZMQ)** | **`daemon_handler::handle(GetInfo)`** | Fills the same **`pos_*` / `pos_pulse_*`** family as unrestricted HTTP (no “restricted RPC” split on this path). | OK |
| **`get_pulse_block_template` / `get_pulse_arqnet_votes`** | **`core_rpc_server`** | Require **`fork_active(net)`**, next height **Pulse slot** (template RPC), ideal HF **`>= network_version_20`**, no bootstrap daemon. | OK |
| **`simplewallet`** | **`start_mining`**, **`status`**, **`refresh`** | Mining refusal on Pulse slots; hybrid telemetry via **`maybe_print_daemon_pos_info`**. | OK |
| **`wallet_rpc_server`** | **`on_start_mining`** | Forwards **`/start_mining`**; **`er.message`** = daemon **`status`** on failure (includes PoS refusal string). | OK |
| **Wallet API (C++)** | **`wallet/api/wallet_manager.cpp`**, **`wallet2_api.h`** | **`startMining` / `stopMining`**: failure → **`errorString()`** (daemon **`status`** or transport). **`daemonPosInfo(DaemonPosInfo&)`**: one **`/getinfo`** round-trip; fills hybrid telemetry mirror fields (**`DaemonPosInfo`**). | OK |
| **`arqmad` CLI `status`** | **`daemon/rpc_command_executor.cpp`** **`show_status()`** | After the main status line, prints **PoW/PoS Hybrid** line when **`pos_fork_active`** or **`pos_planned_fork_height > 0`**. | OK |
| **In-process miner** | **`cryptonote_basic/miner.*`** | No local **`pow_mining_disabled`** guard; mining is expected to start only via RPC / ZMQ paths that already check. | OK (by convention) |

---

## Delivery pack (items 1–4 — operator / maintainer)

1. **HTTP rehearsal (producer-side tooling)** — **`contrib/pulse-rehearsal/pulse_http_rehearsal.py`**: **`get_info`**, **`get_pulse_block_template`**, **`get_pulse_arqnet_votes`**, **`--dump-template-hex`**, **`--submit-if-ready`** (submit when **`merged_pulse_signatures_meet_threshold`**), **`--submit-anyway`** (forced submit), **`--watch-submit`** (poll template with **`--merge-arqnet-votes`** until submit). **`contrib/pulse-rehearsal/pulse_submit_block.py`**: raw **`submit_block`**. Shared **`pulse_tools_common.py`**. E2E example (votes must arrive via SN arqnet first):  
   `python3 contrib/pulse-rehearsal/pulse_http_rehearsal.py --url http://127.0.0.1:19994/json_rpc --watch-submit --merge-arqnet-votes --pulse-round 0 --validator-bitset 1`  
   Single-shot (if threshold already met): **`--pulse-template --merge-arqnet-votes --submit-if-ready`**. **`submit_block`** JSON-RPC errors from **`on_submitblock`** include **duplicate / orphan / verification failed** hints ( **`block_verification_context`** ).
2. **Automated tests** — **`tests/`** tracks the same **standard PoW-era** tree as **`master`** (no Pulse-scoped GTest sources in-tree). Helpers **`contrib/pulse-rehearsal/build_unit_tests.ps1`** / **`build_unit_tests.sh`** configure **`BUILD_TESTS=ON`** and build the full **`unit_tests`** target (expect legacy sources to need porting before they compile against the current core).
3. **Mainnet / governance (no code flip in-tree)** — Follow **[Mainnet go-live checklist](#mainnet-go-live-checklist-enable-powpos-hybrid-on-mainnet)** (`hardfork.cpp` v20 row, constants, economics, rehearsal). This pack does **not** activate mainnet hybrid by itself while the v20 row stays commented.
4. **Anti-spam on arqnet Pulse** — **`arqnet.cpp`**: sliding window per sender hex; limits **`PULSE_QUORUM_PEER_RATE_MAX_EVENTS`** / **`PULSE_QUORUM_PEER_RATE_WINDOW_SEC`** in **`src/arqnet/pulse_wire.h`**. **`pulse_cap`** is not rate-limited.

---

## Commit index (`c8aff143` … `HEAD` on PoS branch at time of writing)

Listed **oldest → newest**. Bodies abbreviated; refer to **`git show <hash>`** for full patches.

| Hash | Subject |
|------|---------|
| `c8aff143` | Scaffold hybrid/Pulse HF (inactive on mainnet) — `arqma_pulse_fork.h`, `network_version_20`, commented mainnet `hardfork.cpp` hooks, regtest pinning. |
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
| `858007bd` | Release build + compiler hygiene (code); immediate docs tip on **`pos`** removes **`summary-pow.md`**, merges maintainer notes into **`summary-pos.md`**. Pulse helpers: **`network_type`**; **`arqnet`** **`mutable`** mutex; **`rolling_median`** move; RPC **`long double`** staking math. |

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
| 2026-05-05 | *(working tree)* | Clear arqnet vote buffer on accepted Pulse block | **`core::add_new_block`**: after success, **`clear_pulse_arqnet_vote_accumulator`** for **`major_version >= network_version_20`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | **`get_info`**: arqnet vote buffer size | **`pos_pulse_arqnet_vote_buffer_blocks`**; **`arqnet_pulse_vote_buffer_distinct_block_count`** + **`core::get_pulse_arqnet_vote_buffer_distinct_block_count`**; ZMQ **`DaemonInfo`**; **`simplewallet`** refresh line; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | **`get_pulse_block_template`**: hash + merge count in reply | Response **`pulse_template_block_hash`**, **`merged_arqnet_vote_count`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | arqnet: forget proposal dedup on vote-buffer clear | **`pulse_seen_recent::forget`** from **`arqnet_pulse_vote_buffer_clear`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | **`merge_arqnet_votes`**: filter by **`validator_bitset`** | RPC merge skips **`vi` < 16** without header bit; **`core_rpc_server.cpp`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | **`get_pulse_block_template`**: threshold hint in reply | **`pulse_signature_threshold`**, **`merged_pulse_signatures_meet_threshold`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | Pack 1–4: HTTP script, tests, docs, arqnet rate limit | **`contrib/pulse-rehearsal/pulse_http_rehearsal.py`**; **`pulse.h`** **`merge_vote_matches_validator_bitset`** + **`pulse_round.cpp`** tests; **`arqnet.cpp`** per-peer Pulse flood cap; **`summary-pos.md`** delivery pack + wire table. |
| 2026-05-05 | *(working tree)* | Rate limit tunables + submit_block script | **`pulse_wire.h`** **`PULSE_QUORUM_PEER_RATE_*`**; **`pulse_submit_block.py`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | Pulse rehearsal: shared json_rpc + dump template hex | **`pulse_tools_common.py`**; **`--dump-template-hex`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | **`build_unit_tests`** helper scripts | **`contrib/pulse-rehearsal/build_unit_tests.ps1`**, **`build_unit_tests.sh`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | **`summary-pos.md`**: PoS integration audit | File-level table (HTTP/ZMQ/wallet/daemon CLI); fix **`simplewallet`** doc (**`status`** vs **`refresh`**); optional follow-ups for **`getblocktemplate`** UX and **`show_status`**. |
| 2026-05-05 | *(working tree)* | PoS UX follow-ups implemented | **`on_getblocktemplate`**: **`CORE_RPC_ERROR_CODE_POW_MINING_DISABLED`** (-14); **`show_status`** PoS line; **`WalletManager`** **`errorString()`** on mining RPC failure; **`summary-pos.md`** audit table updated. |
| 2026-05-05 | *(working tree)* | **`WalletManager::daemonPosInfo`** | **`DaemonPosInfo`** + **`daemonPosInfo()`** in **`wallet2_api.h`** / **`wallet_manager.*`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | CI + rehearsal + **`simplewallet`** refresh | **`.github/workflows/pulse-tests.yml`**; **`pulse_http_rehearsal.py`** extended **`--info`** keys; **`maybe_print_daemon_pos_info`** after **`refresh`**; **`summary-pos.md`**. |
| 2026-05-05 | *(working tree)* | Pulse HTTP E2E + **`submit_block`** errors | **`pulse_http_rehearsal.py`**: **`--watch-submit`**, **`--submit-if-ready`**, **`--submit-anyway`**; **`on_submitblock`** bvc hints; **`summary-pos.md`** technical full path. |
| 2026-05-06 | `858007bd` | Release build + compiler hygiene (AppleClang) | **`blockchain.cpp`**: free helpers **`pulse_coinbase_matches_pulse_header`**, **`get_difficulty_blocks_count`**, **`get_current_diff_target`** take **`network_type`** from **`Blockchain`** (fixes undeclared **`m_nettype`** in file-scope helpers). **`arqnet.cpp`**: **`mutable std::mutex`** so **`distinct_block_count() const`** can lock. **`contrib/epee/include/rolling_median.h`**: member-wise move instead of **`memcpy`** (fixes **`-Wnontrivial-memcall`**). **`rpc_command_executor.cpp`**: service-node share percentages use **`long double`** for **`STAKING_SHARE_PARTS`** division, **`double`** only at API boundaries (avoids precision warning on near-**`UINT64_MAX`** divisor). **Docs (child commit on `pos`)**: former **`summary-pow.md`** content folded into **`summary-pos.md`**; that file removed; changelog/index rows refreshed for **`858007bd`**. |
| 2026-05-06 | *(ci tip on `pos`)* | **`pulse-tests`**: apt **`nettle-dev`** + slim **`unit_tests`** | **`25406313340`**: missing **`hogweed`** for **`libunbound`** **`pkg-config`** — install **`nettle-dev`** on Jammy (not transitional **`libnettle-dev`** / missing **`libhogweed-dev`**). **`25406983520`**: **`unit_tests`** compile failed on legacy **`ban.cpp`** / **`blockchain_db.cpp`** / **`address_from_url.cpp`** vs current core — CI sets **`ARQMA_CI_PULSE_GTEST=ON`** so **`unit_tests`** is **`pulse_gtest_main.cpp`** + **`pulse_round.cpp`** only (**`tests/unit_tests/CMakeLists.txt`**). |
| 2026-05-06 | *(docs)* | Rewrite **`summary-pos.md`**: hybrid activation + **`network_version_20`** naming | Aligns maintainer doc with **default stagenet `network_version_20` HF row**, **`fork_active(STAGENET)`**, legacy **`ARQMA_STAGENET_POS_REHEARSAL`**, and adds **English “rules of operation”** for alternating **PoW / Pulse** slots (**`arqma_pulse_fork.h`** parity formula), mining/RPC split, difficulty branches, **`pos_*`** telemetry (**`fork_active`** not **`FORK_ACTIVE` alone**), **Hybrid integration audit** anchor. |

---

## GitHub Actions status (manual snapshot)

**Update:** **`ci/pulse-unit-tests`** (`.github/workflows/pulse-tests.yml`) was **removed** when **`tests/`** was reset to the **`master`** test tree — there is **no** longer a dedicated Pulse GTest CI job in-tree.

Queried **2026-05-06** with **`gh run list --repo ArqTras/arqma --limit 40`** (historical):

| Workflow | Run (UTC) | Conclusion | Note |
|----------|-----------|--------------|------|
| **`ci/pulse-unit-tests`** *(removed)* | [25406313340](https://github.com/ArqTras/arqma/actions/runs/25406313340) `2026-05-05T22:42:15Z` | **failure** | **`pkg_check_modules(libunbound)`** — missing **`hogweed`**. |
| **`ci/pulse-unit-tests`** *(removed)* | [25406983520](https://github.com/ArqTras/arqma/actions/runs/25406983520) `2026-05-05T23:00:57Z` | **failure** | **`unit_tests`** compile: legacy tests vs current **`cryptonote_protocol`** / **`BlockchainDB`** API. |
| **`ci/gh-actions/depends`** | [25406313329](https://github.com/ArqTras/arqma/actions/runs/25406313329) `2026-05-05T22:42:15Z` | success | Same push as failed Pulse job. |
| **`ci/gh-actions/depends`** | [25403575224](https://github.com/ArqTras/arqma/actions/runs/25403575224) `2026-05-05T21:34:18Z` | success | Earlier **`pos`** push. |

---

## Key paths (quick reference)

- `src/common/arqma_pulse_fork.h` — **`fork_active`**, **`planned_fork_height_for_net`**, **`is_pow_slot_at_height`** / **`is_pulse_slot_at_height`**, **`pow_mining_disabled_for_chain`**, quorum sizes, roadmap.
- `src/cryptonote_basic/cryptonote_basic.h` — Pulse structs and header fields.
- `src/cryptonote_basic/difficulty.cpp` / `.h` — LWMA cores; **`next_difficulty_pulse_pos`**.
- `src/cryptonote_core/blockchain.cpp` / `.h` — **Hybrid** PoW check **only on PoW slots**; Pulse-slot PoW skip; **`verify_pulse_fork_block_rules`**; Pulse coinbase/header checks; template sanitisation; **`create_next_pulse_block_template`**.
- `src/cryptonote_core/pulse.cpp` / `.h` — **`get_round_timings`**, **`convert_time_to_round`**, **`merge_vote_matches_validator_bitset`** (RPC merge / tests).
- `contrib/pulse-rehearsal/pulse_http_rehearsal.py` — HTTP JSON-RPC Pulse rehearsal (**`get_info`**, **`get_pulse_block_template`**, **`get_pulse_arqnet_votes`**).
- `contrib/pulse-rehearsal/pulse_submit_block.py` — JSON-RPC **`submit_block`** for a hex block blob.
- `contrib/pulse-rehearsal/pulse_tools_common.py` — shared **`json_rpc`** for rehearsal scripts.
- `contrib/pulse-rehearsal/build_unit_tests.ps1` / `build_unit_tests.sh` — configure **`BUILD_TESTS=ON`**, build **`unit_tests`** (full tree from **`tests/`**, same as **`master`**).
- `src/cryptonote_core/cryptonote_core.cpp` / `.h` — **`get_pulse_block_template`**, **`copy_pulse_arqnet_vote_accumulator`**, **`clear_pulse_arqnet_vote_accumulator`**, **`get_pulse_arqnet_vote_buffer_distinct_block_count`**.
- `src/cryptonote_core/service_node_list.cpp` / `.h` — **`try_get_block_winner_for_service_node`**, **`validate_miner_tx`** (**pre‑v20**: scheduled **`get_block_winner()`**; **Pulse-slot / hybrid quorum path**: checkpointing-quorum producer at **`height−1`** + payout check).
- `src/crypto/hash.h` — **`hash4`**, **`null_hash4`**.
- `src/rpc/core_rpc_server_error_codes.h` — RPC error codes including **`CORE_RPC_ERROR_CODE_POW_MINING_DISABLED`** (**`-14`**) for **`getblocktemplate`** when PoW is disabled.
- `src/rpc/core_rpc_server*.cpp/.h`, `core_rpc_server_commands_defs.h`, `daemon_handler.cpp`, `message_data_structs.h`, `serialization/json_object.cpp` — telemetry, **`get_pulse_block_template`**, **`get_pulse_arqnet_votes`**, and JSON parity.
- `src/daemon/rpc_command_executor.cpp` — **`show_status`** hybrid summary line from **`get_info`**.
- `src/wallet/api/wallet_manager.cpp`, `wallet2_api.h` — **`WalletManager::errorString()`** after **`startMining` / `stopMining`** failures; **`DaemonPosInfo`** / **`daemonPosInfo()`** (**hybrid** subset of **`get_info`**).
- `src/arqnet/pulse_wire.h`, `src/cryptonote_protocol/arqnet.cpp` — Pulse **arqnet** commands (**`pulse_cap`**, **`pulse_proposal`**, **`pulse_vote`**, **`rh`** relay, in-memory vote buffer).
