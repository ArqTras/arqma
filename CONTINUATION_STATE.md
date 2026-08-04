# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate

- `ninja unit_tests` → **529** passed
- `BUILD_INTEGRATION_TESTS=OFF` (default)

## Cross-platform gates

| Gate | Status |
|------|--------|
| Linux unit + ASan (`ci.yml`) | ✅ |
| macOS-14 unit (`ci.yml`) | ✅ |
| Windows/macOS/Linux depends (`depends.yml`) | ✅ |
| Docs: `PLATFORM.md` + roadmap + PR description | in flight |

## Docs map

- `docs/PLATFORM.md` — OS matrix
- `docs/UPGRADE_ROADMAP.md` — phase / milestone map
- `docs/PR_COMPLETENESS_GATE.md` — merge claims
- `docs/RELEASE_NOTES_upgrade_A.md` — release notes
- `docs/OPERATOR_UPGRADE.md` / `docs/MIGRATION.md`

## Next

1. Milestone C product choice (Storage / Blink / Pulse / Router)
2. Migrate `core_tests` under `BUILD_INTEGRATION_TESTS=ON`
