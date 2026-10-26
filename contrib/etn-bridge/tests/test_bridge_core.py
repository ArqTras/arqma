"""Unit tests for bridge auth + daily limits (no live wallet-rpc)."""

from __future__ import annotations

import os
import sys
import tempfile
import unittest
from pathlib import Path

# Allow `python -m unittest` from contrib/etn-bridge or repo root.
ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))


class BridgeCoreTests(unittest.TestCase):
    def setUp(self) -> None:
        self._tmpdir = tempfile.TemporaryDirectory()
        self.db = Path(self._tmpdir.name) / "swaps.db"
        os.environ["ETN_BRIDGE_DB"] = str(self.db)
        os.environ["ETN_DEMO"] = "1"
        os.environ.pop("ETN_BRIDGE_TOKEN", None)
        os.environ["ETN_DAILY_LIMIT_ATOMIC"] = "0"
        os.environ["ETN_BRIDGE_PAUSED"] = "0"
        # Reload module so env is picked up.
        import importlib

        import etn_bridge.bridge_core as core

        importlib.reload(core)
        self.core = core

    def tearDown(self) -> None:
        self._tmpdir.cleanup()

    def test_token_optional_when_unset(self) -> None:
        self.assertTrue(self.core.token_ok(None))
        self.assertTrue(self.core.token_ok(""))

    def test_token_required_when_set(self) -> None:
        os.environ["ETN_BRIDGE_TOKEN"] = "secret"
        import importlib

        importlib.reload(self.core)
        self.assertFalse(self.core.token_ok(None))
        self.assertFalse(self.core.token_ok("wrong"))
        self.assertTrue(self.core.token_ok("secret"))

    def test_daily_limit(self) -> None:
        os.environ["ETN_DAILY_LIMIT_ATOMIC"] = "1000"
        import importlib

        importlib.reload(self.core)
        self.assertIsNone(self.core.check_daily_limit("mint", "400"))
        swap = self.core.create_swap("mint", "400", "0xabc")
        swap.status = "completed"
        self.core.update_swap(swap)
        err = self.core.check_daily_limit("mint", "700")
        self.assertIsNotNone(err)
        self.assertIn("daily limit exceeded", err)
        self.assertIsNone(self.core.check_daily_limit("mint", "600"))

    def test_pause_flag(self) -> None:
        os.environ["ETN_BRIDGE_PAUSED"] = "1"
        import importlib

        importlib.reload(self.core)
        self.assertIsNotNone(self.core.pause_reason())
        os.environ["ETN_BRIDGE_PAUSED"] = "0"
        importlib.reload(self.core)
        self.assertIsNone(self.core.pause_reason())


if __name__ == "__main__":
    unittest.main()
