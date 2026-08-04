# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate (latest)

- `ninja unit_tests` → **529** passed
- `BUILD_INTEGRATION_TESTS=OFF` (default)

## Cross-platform gates (commit `d44a598a`)

| Gate | Status |
|------|--------|
| Linux unit Release/Debug + ASan (`ci.yml`) | ✅ |
| macOS-14 unit Release (`ci.yml`) | ✅ |
| Format check | ✅ |
| Windows x64 depends (mingw) | ✅ |
| Linux x86_64 / armv8 / RPi depends | ✅ |
| macOS x64 + arm64 depends (clang-19+lld) | ✅ |

## Next exact implementation

1. Migrate legacy `core_tests` under `BUILD_INTEGRATION_TESTS=ON` (default OFF)
2. Prefer local green before every push; strip Co-authored-by
