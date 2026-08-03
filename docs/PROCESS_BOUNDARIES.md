# Process vs Separate Binary Boundaries

Decision record for upgrade-module placement.

## Keep in-process (`arqmad`)

| Component | Why |
|-----------|-----|
| `arqmq` facade + ACL registry | Thin control plane over existing SNNetwork |
| `arq_storage::StorageClient` | Client stubs / RPC status only |
| RPC validation / pagination / DoS caps | Daemon request path |
| Messaging envelope helpers | Shared types for later wire format |

## Separate binary / service

| Component | Why |
|-----------|-----|
| Storage Server | Replication, swarm sync, disk/IO isolation, independent release |
| Privacy router (`arq_router` production) | Network stack lifecycle separate from consensus daemon |
| Messenger clients | UX / mobile / desktop product surfaces |

## Rule of thumb

If a feature needs its own process supervision, storage volume, or upgrade cadence
independent of consensus, it stays out of `arqmad`. Facades and status RPCs may
live in-process as long as they do not pretend to be the full service.
