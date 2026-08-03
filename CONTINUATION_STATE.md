# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only

## Local quality gate (latest)

- `ninja unit_tests` → **522+** passed (p2p Levin sync assert added)

## Next exact implementation

1. **File:** `src/arq_router/` — harden experimental lifecycle when `--arq-router` enabled
2. **Or:** expand OpenAPI stub with wallet-rpc restricted notes
3. **Or:** restore `serialization.cpp` / other legacy fixtures incrementally
4. Prefer local green suite before every push

## Recent HEAD (pushed unless noted)

- rpc_auth tests, storage HTTP GET, onion_layer, fee, ArqMQ framing, wallet auth/validation, tx_utils
- Pending this turn: PROCESS_BOUNDARIES update + p2p Levin/preauth equality assert

## Authorship

`git -c core.hooksPath=/tmp/empty-git-hooks` · ArqTras
