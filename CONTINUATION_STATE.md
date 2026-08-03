# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate (latest)

- `ninja unit_tests` → **529** passed (~2.0s)
- `ctest -R 'unit_tests|hash-target'` → green
- `BUILD_INTEGRATION_TESTS=OFF` (default)

## Cross-platform gates (commit `2ef4a62c` + follow-ups)

| Gate | Status |
|------|--------|
| Linux unit Release/Debug (`ci.yml`) | ✅ |
| macOS-14 unit Release (`ci.yml`) | ✅ |
| Linux ASan/UBSan (`ci.yml`) | ✅ |
| Format check (`ci.yml`) | ✅ |
| Windows x64 depends (mingw) | ✅ |
| Linux x86_64 / armv8 depends | ✅ |
| macOS depends cross (zeromq `-std=c++17`) | 🔧 fix in flight |

## Next exact implementation

1. Land zeromq darwin CXXFLAGS fix; confirm `build-depends-macOS-*` green
2. Migrate legacy `core_tests` under `BUILD_INTEGRATION_TESTS=ON` (default OFF)
3. Prefer local green before every push; strip Co-authored-by
