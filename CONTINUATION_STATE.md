# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate (latest)

- `ninja unit_tests` → **529** passed (~2.0s)
- `ctest -R 'unit_tests|hash-target'` → green
- `BUILD_INTEGRATION_TESTS=OFF` (default) — legacy `core_tests` not built in CI

## Cross-platform gates

- **Linux/macOS unit CI:** `.github/workflows/ci.yml` (`unit-tests` matrix + format + sanitize)
- **Windows (+ cross macOS/Linux binaries):** `.github/workflows/depends.yml` (`build-depends-windows-x64` mingw)

## Next exact implementation

1. Migrate/fix legacy `core_tests` incrementally under `BUILD_INTEGRATION_TESTS=ON` (keep default OFF)
2. Optional: native Windows MSVC unit CI once Boost/OpenSSL are pinned for `windows-latest`
3. Prefer local green before every push; strip Co-authored-by

## Architecture (locked decisions)

See CHANGELOG-upgrade.md and prior commits on `upgrade`. Do not re-run phase-1 analysis.
