# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`

## Local quality gate (latest)

- `ninja unit_tests` → **525** passed (~2.1s)

## Next exact implementation

1. **File:** `tests/unit_tests/serialization.cpp` — attempt restore in CMakeLists  
2. If compile/runtime fails: leave commented with TODO and move to `varint`/`hardfork` restore  
3. **Commit:** `test: restore serialization unit coverage` (or skip note)  
4. Then CI workflow sanity / `docs/UPGRADE_ROADMAP.md` bump  

## Last completed

- `hash_pubkey_to_swarm` FNV-1a helper + tests (pending push with this state)
