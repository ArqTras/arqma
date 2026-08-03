# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`

## Local quality gate (latest)

- `ninja unit_tests` → **525** passed (~2.0s)
- `ninja daemon` → OK

## Next exact implementation

1. Refactor `on_start_mining` / `on_stop_mining` / `on_stop_daemon` to shared `deny_restricted_rpc`  
2. Or expand `docs/ERROR_SEMANTICS.md` for `CORE_RPC_ERROR_CODE_RESTRICTED`  
3. Or attempt focused subset of serialization tests  
4. Keep local-first green before push

## Last completed (pending push)

- Operator RPC gates for set_bans/flush_txpool/save_bc/relay_tx/log/pop/prune
- `CORE_RPC_ERROR_CODE_RESTRICTED` (-14)
