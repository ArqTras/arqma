# ArqMQ Migration Plan

## Goal

Evolve Arq-Net messaging toward a maintainable command/ACL/worker model inspired
by OxenMQ, while preserving Arqma naming, ports and operator UX.

## Current state

`src/arqnet` provides the live Curve/ZMQ mesh (`SNNetwork`).

`src/arqmq` is the Arqma-named facade:

- `--arqnet-backend=legacy-arqnet` or `arqmq` both initialize successfully
- wire transport name reported by RPC/status is always `snnetwork` today
- command ACL registry documents intended privilege levels
- a dedicated ArqMQ socket stack remains future work (Milestone C / later)

## Migration steps

1. ~~Extract stable facade headers from `SNNetwork` usage sites.~~
2. ~~Add feature flag `--arqnet-backend=legacy-arqnet|arqmq` (daemon arg).~~
3. Vendor OxenMQ **or** port selected subsystems under Arqma copyright/license
   notices without Co-Authored-By trailers (remaining).
4. Dual-run on testnet/stagenet.
5. Make dedicated `arqmq` transport default; keep SNNetwork until one stable release.
6. Optionally require Arq-Net ping for uptime proofs after operator notice.

## Target architecture

```
arqmad
  └── arqnet facade (stable C++ API for cryptonote_core / protocol)
        └── transport backend
              ├── legacy SNNetwork (default during migration)
              └── arqmq backend (feature-flagged)
```

Public names remain **Arq-Net** / **arqnet**. Internal library may be called
`arqmq` in code comments and CMake targets if split out.

## ACL categories (planned)

| Category | Who | Examples |
|----------|-----|----------|
| `Denied` | rejected | unknown Curve peers |
| `Basic` | local admin / loopback helpers | diagnostics |
| `ServiceNode` | registered SN keys | quorum votes, pings |
| `Admin` | local control socket | privileged ops |

`arqmq::allows(required, granted)` encodes the privilege order
`Denied < Basic < ServiceNode < Admin` for future command routing.

Unknown remote Curve keys stay denied (already enforced).

### Framing limits (`message_limits.hpp`)

Aligned with `SN_ZMQ_MAX_MSG_SIZE` (1 MiB):

| Limit | Value |
|-------|-------|
| `max_message_bytes` | 1048576 |
| `max_command_name_bytes` | 64 |
| `max_payload_frames` | 16 |

`authorize_request` combines framing checks with ACL authorize; Arq-Net
`vote_ob` / `ping` / `pong` handlers use it.

## Migration steps

1. Extract stable facade headers from `SNNetwork` usage sites.
2. Add feature flag `--arqnet-backend=legacy-arqnet|arqmq` (daemon arg).
3. Vendor OxenMQ **or** port selected subsystems under Arqma copyright/license
   notices without Co-Authored-By trailers.
4. Dual-run on testnet/stagenet.
5. Make `arqmq` default; keep legacy until one stable release cycle.
6. Optionally require Arq-Net ping for uptime proofs after operator notice.

## Non-goals

- Renaming the product mesh to Lokinet/Oxennet
- Breaking Levin P2P
- Shipping Session branding

## Success criteria

- Existing quorum vote relay works unchanged for operators
- Unit/integration tests cover auth deny path and ping RPC
- CI builds both backends when flag is present
- Docs updated in `docs/ARQNET.md` and release notes
