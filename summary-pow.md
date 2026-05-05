# Arqma — PoW baseline / build maintenance notes

This document records **non-consensus** fixes and tooling notes around the **Proof-of-Work (PoW)** build path and shared libraries (e.g. wallet/daemon code paths that are not PoS-specific). For PoS / Pulse feature history, see **`summary-pos.md`**.

**Maintainers:** Append new bullets under **[Changelog (append-only)]** with UTC date (or sprint), commit hash, one-line summary, and a short technical note.

---

## Changelog (append-only)

- **2026-05-06** — **Build + compiler hygiene (Release, AppleClang)**  
  - **`src/cryptonote_core/blockchain.cpp`**: Free helpers (`pulse_coinbase_matches_pulse_header`, `get_difficulty_blocks_count`, `get_current_diff_target`) took **`m_nettype`** from nowhere; **`network_type`** is now passed explicitly from **`Blockchain`** call sites so Release builds compile.  
  - **`src/cryptonote_protocol/arqnet.cpp`**: **`distinct_block_count() const`** locked a non-**`mutable`** mutex — fixed with **`mutable std::mutex`** on the vote accumulator.  
  - **`contrib/epee/include/rolling_median.h`**: Replaced **`memcpy`**-based move ctor/assign (Clang **`-Wnontrivial-memcall`**) with explicit member-wise move and **`noexcept`**.  
  - **`src/daemon/rpc_command_executor.cpp`**: Service-node share percentages avoid converting **`STAKING_SHARE_PARTS`** straight to **`double`** (precision warning); use **`long double`** intermediates and **`static_cast<double>`** only where the API requires floats.
