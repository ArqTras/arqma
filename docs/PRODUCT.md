# Arqma product (one repository)

This tree is the **unified Arqma product**. Consensus, storage, privacy routing,
and messenger helpers ship together. They stay **separate processes** (see
[`PROCESS_BOUNDARIES.md`](PROCESS_BOUNDARIES.md)); they are no longer split
across other repositories.

## Binaries

| Binary | Role |
|--------|------|
| `arqmad` | Consensus daemon (RandomARQ + hybrid Pulse, Arq-Net, RPC) |
| `arqma-wallet-rpc` / `arqma-wallet-cli` | Wallet processes |
| `arqma-storage` | HTTP Storage Server (KV + TTL + `--data-dir` + `--peer` / swarm replicas) |
| `arqma-router` | Privacy-router (`POST /v1/peel`, `POST /v1/store` into storage) |
| `arqma-msg` | CLI: `gen` / `send` / `get` / `inbox` / `open` (optional `--router`) |

Blink lives **in-process** as `src/arq_blink` (quorum sign/verify + collector).
It pre-confirms transactions. It does **not** replace Pulse or RandomARQ.

## Local stack

```text
utils/arqma-stack.sh                 # Linux / macOS
utils/arqma-stack.cmd                # Windows
arqmad --storage-client-url=http://127.0.0.1:22021 --arq-router
arqma-msg gen
arqma-msg send --url http://127.0.0.1:22021 --to <64-hex> --text hello
arqma-msg send --router http://127.0.0.1:1090 --to <64-hex> --text hello
arqma-msg inbox --url http://127.0.0.1:22021 --to <64-hex>
arqma-msg open --to <pub> --secret <priv> --key <id>
```

`PUT /v1/kv?ttl=` expires values (cap 14 days; `0` means keep). Inbox namespaces
`inbox-<pubkey>` also fan out to URLs in `PUT /v1/snodes`. `GET /v1/swarm?pubkey=`
returns the FNV swarm id plus those member URLs. HTTP bodies are capped at 1 MiB;
listen addresses accept IPv6 (`[::1]:22021`).

`get_storage_status` / `storage_server_ping` talk to `arqma-storage`.
`get_blink_status` / `print_blink` report the in-daemon Blink collector.

## Locks that stay

1. Hybrid PoW + Pulse. RandomARQ remains required. Pulse stage stays **2**.
2. Default `--arqnet-backend=legacy-arqnet`.
3. Storage and router stay out of the `arqmad` process.
4. No Oxen / Session / Lokinet branding or tokenomics copied in.
