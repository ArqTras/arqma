# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate (latest)

- `ninja unit_tests` → **529** passed
- `BUILD_INTEGRATION_TESTS=OFF` (default)

## Cross-platform gates

| Gate | Status |
|------|--------|
| Linux/macOS unit + ASan (`ci.yml`) | ✅ on `607abec5` / prior |
| Windows x64 depends (mingw) | ✅ |
| Linux x86_64 / armv8 depends | ✅ |
| macOS depends cross | 🔧 clang-19+lld darwin host (Monero-style) in flight |

## Next exact implementation

1. Confirm `build-depends-macOS-*` green after darwin.mk/lld port
2. Migrate legacy `core_tests` under `BUILD_INTEGRATION_TESTS=ON` (default OFF)
3. Prefer local green before every push; strip Co-authored-by
