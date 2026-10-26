# Arqma Cipherstorm Vault v10.1.0 — Upgrade foundation

Software upgrade for the daemon / SN stack. **No new mainnet hard fork activates in this release.**
Mainnet stays on HF19 wire until height **4 000 000** (HF20 hybrid mesh + Pulse permit), then **5 000 000** (HF21 exclusive mesh; hybrid PoW). RandomARQ remains required. Pulse stays hybrid (stage 2). Default `--arqnet-backend=legacy-arqnet`.

| Field | Value |
|-------|--------|
| Tag (when cut) | `v10.1.0` |
| Semver (`src/version.cpp.in`) | `10.1.0` |
| Codename (`ARQMA_RELEASE_NAME`) | `Cipherstorm Vault` |
| Consensus | Still **HF19** on mainnet until height **4 000 000** (HF20) / **5 000 000** (HF21) |

## Who should update

- Recommended for **all full nodes and service nodes** before the HF20 window.
- Wallet operators: use CLSAG-capable `arqma-wallet-cli` / `arqma-wallet-rpc` from this tree.
- Optional: `arqma-storage` / `arqma-router` / `arqma-msg` operator probes (separate PIDs).

## Highlights

`[+]` core: HF19 burn construction fixed; HF20/HF21 gates reserved (mainnet heights unchanged)
`[+]` arqnet: registered SN Curve auth; restore `arqnet_ping` / `last_arqnet_ping`
`[+]` pulse: hybrid producer wiring (SN sign / gossip / retransmit); PoW still required
`[+]` build: C++20 baseline; curated unit suite (628) + 3-OS depends CI
`[*]` rpc: restricted-RPC catalog, DoS soft-caps, OpenAPI stub
`[*]` ops: companion HTTP servers shut down cleanly; Blink collector (local observability)

## Do not flip (mainnet locks)

- Default `--arqnet-backend` (stays `legacy-arqnet`)
- Pulse stage 3 / PoW-off
- HF20 / HF21 heights
- Merging storage/router into `arqmad`

## Platforms / binaries

| OS | Arch | Artifact (proposed) |
|----|------|---------------------|
| Linux | x64 | `arqma-linux-x64-release-v10.1.0[<sha7>].tar.bz2` |
| Linux | arm64 | `arqma-linux-arm64-release-v10.1.0[<sha7>].tar.bz2` |
| Linux | armv8 (RPi / NO_AES) | `arqma-linux-armv8-release-v10.1.0[<sha7>].tar.bz2` |
| macOS | x64 | `arqma-macos-x64-release-v10.1.0[<sha7>].tar.bz2` |
| macOS | arm64 | `arqma-macos-arm64-release-v10.1.0[<sha7>].tar.bz2` |
| Windows | x64 | `arqma-win-x64-release-v10.1.0[<sha7>].zip` |

Also publish `SHA256SUMS` for the set. Prefer depends-built trees for self-contained operators.

Package contents: `arqmad`, wallets, optional probes — see `docs/OPERATOR_UPGRADE.md` / `docs/BINARIES.md`.

## Verify

```
sha256sum -c SHA256SUMS
```

Docs: `docs/RELEASE_NOTES_upgrade_A.md`, `docs/OPERATOR_UPGRADE.md`, `docs/UPGRADE_ROADMAP.md`, `docs/PLATFORM.md`.

## Krótki opis PL

Arqma Cipherstorm Vault v10.1.0 — fundament upgrade’u (PR modernizacji).
Bez nowego hard forka na mainnecie: nadal HF19 do bloku 4 000 000 (HF20), potem 5 000 000 (HF21).
Pulse hybrydowy, RandomARQ obowiązkowy, domyślny `--arqnet-backend=legacy-arqnet`.
Zalecana aktualizacja pełnych węzłów i SN przed oknem HF20. Binarki: Linux/macOS/Windows (x64/arm).
