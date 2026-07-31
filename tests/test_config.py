import os
import shutil
import subprocess
from pathlib import Path


def find_tool(name: str) -> Path | None:
    """Locate an executable in PATH or common install locations."""
    which = shutil.which(name)
    if which:
        return Path(which)
    # Common Windows locations for Ghostscript
    gs_candidates = [
        Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "gs",
        Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "gs",
    ]
    if name.lower() in ("gswin64c", "gswin64", "gs"):
        for root in gs_candidates:
            for ver in sorted(root.iterdir(), reverse=True) if root.is_dir() else []:
                exe = ver / "bin" / f"{name}.exe"
                if exe.is_file():
                    return exe
    return None


def detect_tools():
    tools = {}
    for exe in ("jbig2dec", "gs", "gswin64c", "gswin64"):
        tools[exe] = find_tool(exe)

    # ImageMagick
    magick = find_tool("magick")
    if magick:
        # Try reading version to confirm it works
        try:
            subprocess.run(
                [str(magick), "--version"],
                capture_output=True,
                timeout=10,
                check=False,
            )
            tools["magick"] = magick
        except Exception:
            pass
    else:
        tools["magick"] = None

    return tools


TOOLS = detect_tools()

ROOT = Path(__file__).resolve().parent.parent

# Path to the jbig2 executable.  Override with the JBIG2_EXE environment
# variable (used by CI / tests/run.py); otherwise fall back to the default
# MSVC build location on Windows.
JBIG2_EXE = (
    Path(os.environ.get("JBIG2_EXE", ""))
    if os.environ.get("JBIG2_EXE")
    else ROOT / "build.msvc.Release" / "Release" / "jbig2.exe"
)

# Test fixtures live under images/ and are committed to the repo (force-added
# past the *.png/*.jpg/*.tif gitignore rules; see images/Readme.md).  Using
# committed fixtures keeps the suite self-contained on a fresh checkout and in
# CI, with no external downloads.
TEST_IMAGE_PNG = ROOT / "images" / "15.png"  # text page (no graphics)
TEST_IMAGE_TIF = ROOT / "images" / "feyn.tif"  # text page, large
TEST_IMAGE_JPG = ROOT / "images" / "amoris.2.150.jpg"  # grayscale page w/ graphics
TEST_IMAGE_PHOTO = ROOT / "images" / "1555.003.jpg"  # photo-heavy page (issue #142)

# Directory containing the leptonica shared libraries, prepended to PATH so
# the jbig2 executable can find them at runtime.  Optional (only used when it
# exists); override with the LEPTONICA_BIN environment variable.
LEPTONICA_BIN = Path(os.environ.get("LEPTONICA_BIN", "F:/win64/bin"))
