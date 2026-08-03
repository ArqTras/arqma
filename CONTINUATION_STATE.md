# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate (latest)

- `ninja unit_tests` → **524** passed (~2.0s)
- HEAD: `c450aa6b` (OpenAPI stub expansion)

## Next exact implementation

1. **File:** `src/arq_messaging/swarm_map.hpp`  
2. **Function:** `swarm_id hash_pubkey_to_swarm(std::string_view pubkey)` deterministic  
3. **Tests:** in `arq_upgrade_modules.cpp`  
4. **Commit:** `feat: add deterministic pubkey-to-swarm_id helper`  
5. Then evaluate restoring `serialization.cpp` unit fixture  
6. Keep local-first green before every push

## Architecture (do not re-analyze)

ArqMQ ACL+framing · messaging onion/sealed · storage HTTP GET · wallet
validation+auth · router validate_config · rpc_auth tests · fee/tx_utils ·
OpenAPI 0.2.0 · Levin=P2P_PREAUTH lock · swarm membership caps
