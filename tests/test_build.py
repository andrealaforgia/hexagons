"""Check that header edits trigger real incremental recompilation."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def touch(path):
    # This make compares timestamps to the second, and a small build finishes
    # within the second in which the header is edited.
    time.sleep(1.1)
    path.touch()


with tempfile.TemporaryDirectory(prefix="hexagons-build-test-") as directory:
    checkout = Path(directory) / "hexagons"
    shutil.copytree(
        ROOT,
        checkout,
        ignore=shutil.ignore_patterns(
            ".git", "*.o", "*.d", "*.dSYM", "__pycache__", "hexagons"
        ),
    )
    subprocess.run(["make", "-s", "-j4"], cwd=checkout, check=True)
    assert subprocess.run(["make", "-q"], cwd=checkout).returncode == 0
    touch(checkout / "game/src/main/game.h")
    assert (
        subprocess.run(["make", "-q"], cwd=checkout).returncode == 1
    ), "Game header edit did not trigger rebuilding"
    subprocess.run(["make", "-s", "-j4"], cwd=checkout, check=True)
    touch(checkout / "engine/core/math/geometry.h")
    assert (
        subprocess.run(["make", "-q"], cwd=checkout).returncode == 1
    ), "Engine header edit did not trigger rebuilding"
    subprocess.run(["make", "-s", "-j4"], cwd=checkout, check=True)
    assert subprocess.run(["make", "-q"], cwd=checkout).returncode == 0
print("PASS incremental rebuilds after game and engine header edits")
