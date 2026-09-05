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
| `arqma-storage` | HTTP Storage Server (SN uptime probe + KV / swarm list; `--data-dir` volume) |
| `arqma-router` | Privacy-router process (HTTP status + `POST /v1/peel`; separate from `arqmad`) |
| `arqma-msg` | CLI messenger: `gen` / `send` / `get` / `inbox` via storage + sealed envelope |

Blink lives **in-process** as `src/arq_blink` (quorum sign/verify + collector).
It pre-confirms transactions. It does **not** replace Pulse or RandomARQ.

## Local stack

```text
utils/arqma-stack.sh                 # arqma-storage + arqma-router
arqmad --storage-client-url=http://127.0.0.1:22021 --arq-router
arqma-msg gen
arqma-msg send --url http://127.0.0.1:22021 --to <64-hex> --text hello
arqma-msg inbox --url http://127.0.0.1:22021 --to <64-hex>
```

`get_storage_status` / `storage_server_ping` talk to `arqma-storage`.
`get_blink_status` / `print_blink` report the in-daemon Blink collector.

## Locks that stay

1. Hybrid PoW + Pulse. RandomARQ remains required. Pulse stage stays **2**.
2. Default `--arqnet-backend=legacy-arqnet`.
3. Storage and router stay out of the `arqmad` process.
4. No Oxen / Session / Lokinet branding or tokenomics copied in.
