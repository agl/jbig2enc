#!/usr/bin/env python3
"""Convenience runner for the jbig2enc test suite.

Usage:
    python tests/run.py                    # run all tests
    python tests/run.py -v                 # verbose
    python tests/run.py TestJbig2Basic     # specific test class
    python tests/run.py --list             # list test classes

Environment:
    JBIG2_EXE: path to jbig2.exe (default: build.msvc.Release/Release/jbig2.exe)
"""

import os
import sys
import unittest

# Ensure tests/ is importable even when run from repo root
_test_dir = os.path.dirname(os.path.abspath(__file__))
_root = os.path.dirname(_test_dir)
sys.path.insert(0, _root)

if __name__ == "__main__":
    args = sys.argv[1:]
    loader = unittest.TestLoader()

    if "--list" in args:
        suite = loader.discover(os.path.dirname(__file__), pattern="test_*.py")
        seen = set()
        for case in suite:
            for cls in case._tests:
                name = (
                    type(cls._tests[0]).__name__
                    if hasattr(cls, "_tests")
                    else type(cls).__name__
                )
                if name not in seen:
                    print(name)
                    seen.add(name)
        sys.exit(0)

    if args and not args[0].startswith("-"):
        # Run specific test class or method
        module_name = "test_jbig2"
        for i, arg in enumerate(args):
            if arg.startswith("Test"):
                suite = loader.loadTestsFromName(f"tests.{module_name}.{arg}")
                args.pop(i)
                break
        else:
            suite = loader.discover(os.path.dirname(__file__), pattern="test_*.py")
    else:
        suite = loader.discover(os.path.dirname(__file__), pattern="test_*.py")

    runner = unittest.TextTestRunner(verbosity=2 if "-v" in args else 1)
    result = runner.run(suite)
    sys.exit(0 if result.wasSuccessful() else 1)
