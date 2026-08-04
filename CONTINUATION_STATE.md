# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate

- `ninja unit_tests` → **531** passed
- `ninja daemon` → OK
- `BUILD_INTEGRATION_TESTS=OFF` (default)

## Cross-platform gates

| Gate | Status |
|------|--------|
| Linux unit + ASan (`ci.yml`) | ✅ |
| macOS-14 unit (`ci.yml`) | ✅ |
| Windows/macOS/Linux depends (`depends.yml`) | ✅ |
| Docs: `PLATFORM.md` + roadmap + PR description | ✅ |

## Foundation gaps closed in latest push

- Wallet restricted RPC: transfers CSV, proofs, create/open/close/restore
- Checkpoints unit tests restored (null-safe `m_db`)
- `--arq-router` lives for daemon lifetime
- OpenAPI aligned to JSON-RPC methods
- Flag docs: `legacy-arqnet`, `--storage-client-url`

## Next (post-foundation)

1. Milestone C product choice (Storage / Blink / Pulse / Router)
2. Migrate `core_tests` under `BUILD_INTEGRATION_TESTS=ON`
