import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

# fmt: off
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tests.test_config import (JBIG2_EXE, LEPTONICA_BIN, ROOT, TEST_IMAGE_JPG,
                               TEST_IMAGE_PHOTO, TEST_IMAGE_PNG,
                               TEST_IMAGE_TIF, TOOLS)
# fmt: on

# ── helpers ──────────────────────────────────────────────────────────────────


def _env():
    env = os.environ.copy()
    if LEPTONICA_BIN.is_dir():
        env["PATH"] = str(LEPTONICA_BIN) + os.pathsep + env["PATH"]
    return env


def _run(*args, timeout=120):
    cmd = [str(JBIG2_EXE)]
    cmd.extend(str(a) for a in args)
    proc = subprocess.run(
        cmd,
        capture_output=True,
        timeout=timeout,
        env=_env(),
    )
    return proc


def _run_cwd(cwd, *args, timeout=120):
    cmd = [str(JBIG2_EXE)]
    cmd.extend(str(a) for a in args)
    proc = subprocess.run(
        cmd,
        capture_output=True,
        timeout=timeout,
        cwd=cwd,
        env=_env(),
    )
    return proc


def _require_jbig2():
    if not JBIG2_EXE.is_file():
        raise unittest.SkipTest(f"jbig2.exe not found at {JBIG2_EXE}")


def _require_image(path: Path):
    if not path.is_file():
        raise unittest.SkipTest(f"test image not found: {path}")


JBIG2_MAGIC = b"\x97\x4a\x42\x32\x0d\x0a\x1a\x0a"


# ── test classes ─────────────────────────────────────────────────────────────


class TestJbig2Basic(unittest.TestCase):
    """Basic sanity tests for the jbig2 CLI."""

    def setUp(self):
        _require_jbig2()

    def test_version(self):
        """-V prints version to stderr."""
        proc = _run("-V")
        self.assertEqual(proc.returncode, 0)
        stderr = proc.stderr.decode("utf-8", errors="replace")
        self.assertIn("jbig2enc", stderr)

    def test_help(self):
        """-h prints usage."""
        proc = _run("-h")
        self.assertEqual(proc.returncode, 0)
        out = (proc.stdout + proc.stderr).decode("utf-8", errors="replace")
        self.assertIn("Usage:", out)


class TestJbig2GenericMode(unittest.TestCase):
    """Generic (non-symbol) mode compression (all output to stdout)."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_generic_compress_png(self):
        """Compress a 1bpp PNG -> stdout with JBIG2 magic."""
        proc = _run(TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)
        self.assertTrue(proc.stdout.startswith(JBIG2_MAGIC))

    def test_generic_compress_tif(self):
        """Compress a 1bpp TIFF -> stdout."""
        proc = _run(TEST_IMAGE_TIF)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)
        self.assertTrue(proc.stdout.startswith(JBIG2_MAGIC))


class TestJbig2SymbolMode(unittest.TestCase):
    """Symbol mode (-s) compression."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_symbol_mode_stdout(self):
        """-s without -p writes JBIG2 data to stdout."""
        proc = _run("-s", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)
        self.assertTrue(proc.stdout.startswith(JBIG2_MAGIC))

    def test_symbol_mode_pdf_ready(self):
        """-s -p writes PDF-ready files (.sym, .0000) to cwd."""
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, "-s", "-p", str(TEST_IMAGE_PNG))
            self.assertEqual(proc.returncode, 0)
            self.assertTrue((Path(tmp) / "output.sym").is_file(), "output.sym missing")
            self.assertTrue(
                (Path(tmp) / "output.0000").is_file(), "output.0000 missing"
            )

    def test_symbol_mode_pdf_ready_custom_name(self):
        """-s -p with -b basename writes basename.sym and basename.0000."""
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, "-s", "-p", "-b", "mybook", str(TEST_IMAGE_PNG))
            self.assertEqual(proc.returncode, 0)
            self.assertTrue((Path(tmp) / "mybook.sym").is_file())
            self.assertTrue((Path(tmp) / "mybook.0000").is_file())

    def test_symbol_mode_auto_threshold(self):
        """-s -a (auto threshold) works."""
        proc = _run("-s", "-a", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)

    def test_symbol_mode_with_threshold(self):
        """-s -t <val> sets classification threshold."""
        proc = _run("-s", "-t", "0.85", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)

    def test_symbol_mode_with_weight(self):
        """-s -w <val> sets classification weight."""
        proc = _run("-s", "-w", "0.6", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)

    def test_emit_json(self):
        """--emit-json (with -s) writes a glyph provenance sidecar."""
        import json

        _require_image(TEST_IMAGE_PNG)
        _require_image(TEST_IMAGE_JPG)
        with tempfile.TemporaryDirectory() as tmp:
            sidecar = Path(tmp) / "provenance.json"
            proc = _run_cwd(
                tmp,
                "--emit-json",
                sidecar,
                "-s",
                "-p",
                str(TEST_IMAGE_PNG),
                str(TEST_IMAGE_JPG),
            )
            self.assertEqual(proc.returncode, 0, proc.stderr.decode())
            self.assertTrue(sidecar.is_file(), "sidecar missing")
            data = json.loads(sidecar.read_text())

            self.assertEqual(data["version"], 1)

            # two input images: page association and per-page geometry must
            # both survive (a regression that assigns everything to page 1
            # or copies page 1's geometry must fail here)
            self.assertEqual(data["num_pages"], 2)
            self.assertEqual(len(data["pages"]), 2)
            self.assertEqual([p["page"] for p in data["pages"]], [1, 2])
            from PIL import Image as PILImage

            for page, image in zip(data["pages"], (TEST_IMAGE_PNG, TEST_IMAGE_JPG)):
                width, height = PILImage.open(str(image)).size
                self.assertEqual(page["width"], width)
                self.assertEqual(page["height"], height)
                self.assertGreater(page["xres"], 0)

            self.assertEqual(data["num_symbols"], len(data["symbols"]))
            self.assertEqual(data["num_instances"], len(data["instances"]))
            self.assertGreater(data["num_symbols"], 0)
            self.assertGreater(data["num_instances"], 0)
            seen_pages = {inst["page"] for inst in data["instances"]}
            self.assertEqual(seen_pages, {1, 2})

            page_by_id = {p["page"]: p for p in data["pages"]}
            heights = {s["class"]: s["height"] for s in data["symbols"]}
            for sym in data["symbols"]:
                self.assertGreater(sym["width"], 0)
                self.assertGreater(sym["height"], 0)
            for inst in data["instances"]:
                self.assertIn(inst["class"], heights)
                page = page_by_id[inst["page"]]
                # ul and ll are the corners of the placement box in raster
                # coordinates (origin at the top left, y down)
                self.assertGreaterEqual(inst["ll"][1], inst["ul"][1])
                self.assertAlmostEqual(
                    inst["ll"][1] - inst["ul"][1], heights[inst["class"]], delta=2
                )
                self.assertLessEqual(inst["ll"][0], page["width"])
                self.assertLessEqual(inst["ll"][1], page["height"])

    def test_emit_json_requires_symbol_mode(self):
        """--emit-json without -s is rejected."""
        _require_image(TEST_IMAGE_PNG)
        proc = _run("--emit-json", "-", str(TEST_IMAGE_PNG))
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("symbol mode", proc.stderr.decode())


class TestJbig2DuplicateLineRemoval(unittest.TestCase):
    """TPGD duplicate-line-removal flag."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_duplicate_line_removal(self):
        """-d does not crash and produces valid magic."""
        proc = _run("-d", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertTrue(proc.stdout.startswith(JBIG2_MAGIC))


class TestJbig2GrayScaleInput(unittest.TestCase):
    """Test with grayscale/color input images."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_JPG)

    def test_grayscale_jpeg(self):
        """8-bit JPEG input is auto-thresholded and compressed."""
        proc = _run("-s", TEST_IMAGE_JPG)
        self.assertEqual(proc.returncode, 0, "JPEG symbol-mode failed")
        self.assertGreater(len(proc.stdout), 0)

    def test_grayscale_global_threshold(self):
        """-G -T <val> sets global binarization threshold."""
        proc = _run("-s", "-G", "-T", "128", TEST_IMAGE_JPG)
        self.assertEqual(proc.returncode, 0)


class TestJbig2Upsampling(unittest.TestCase):
    """Upsampling flags (-2, -4)."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_upsample_2x(self):
        proc = _run("-2", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertTrue(proc.stdout.startswith(JBIG2_MAGIC))

    def test_upsample_4x(self):
        proc = _run("-4", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertTrue(proc.stdout.startswith(JBIG2_MAGIC))


class TestJbig2LargeImage(unittest.TestCase):
    """Test with a large image (2528x3300)."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_TIF)

    def test_large_tiff_generic(self):
        """Large TIFF in generic mode."""
        proc = _run(TEST_IMAGE_TIF)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)

    def test_large_tiff_symbol(self):
        """Large TIFF in symbol mode."""
        proc = _run("-s", TEST_IMAGE_TIF)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)


class TestJbig2DpiOverride(unittest.TestCase):
    """DPI override flag (-D)."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_dpi_override(self):
        """-D <dpi> sets output DPI."""
        proc = _run("-D", "300", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)


class TestJbig2HashMode(unittest.TestCase):
    """--no-hash flag for auto thresholding."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_no_hash(self):
        """--no-hash works (must be used with -s -a)."""
        proc = _run("-s", "-a", "--no-hash", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)


class TestJbig2Jbig2topdf(unittest.TestCase):
    """jbig2topdf.py PDF assembly from compressed data."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def _run_pdf_script(self, tmp, *args):
        script = ROOT / "jbig2topdf.py"
        cmd = [sys.executable, str(script)]
        cmd.extend(str(a) for a in args)
        return subprocess.run(
            cmd,
            capture_output=True,
            timeout=30,
            cwd=tmp,
            env=_env(),
        )

    def test_jbig2topdf_with_basename(self):
        """jbig2topdf.py produces PDF from .sym and .0000 files."""
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, "-s", "-p", "-b", "doc", str(TEST_IMAGE_PNG))
            self.assertEqual(proc.returncode, 0)
            pdf_proc = self._run_pdf_script(tmp, "doc")
            self.assertEqual(pdf_proc.returncode, 0)
            self.assertGreater(len(pdf_proc.stdout), 0)
            self.assertTrue(pdf_proc.stdout.startswith(b"%PDF-"))

    def test_jbig2topdf_standalone(self):
        """jbig2topdf.py -s (standalone mode) from raw page files."""
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, str(TEST_IMAGE_PNG))
            page = Path(tmp) / "page-0.jb2"
            page.write_bytes(proc.stdout)
            pdf_proc = self._run_pdf_script(tmp, "-s", page.name)
            self.assertEqual(pdf_proc.returncode, 0)
            self.assertGreater(len(pdf_proc.stdout), 0)
            self.assertTrue(pdf_proc.stdout.startswith(b"%PDF-"))


class TestJbig2Verbose(unittest.TestCase):
    """Verbose output (-v)."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_verbose_generic(self):
        """-v prints image info on stderr."""
        proc = _run("-v", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        stderr = proc.stderr.decode("utf-8", errors="replace")
        self.assertIn("image:", stderr)

    def test_verbose_symbol(self):
        """-s -v prints image info on stderr."""
        proc = _run("-s", "-v", TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        stderr = proc.stderr.decode("utf-8", errors="replace")
        self.assertIn("image:", stderr)

    def test_verbose_large(self):
        """Verbose output with large TIFF."""
        proc = _run("-v", TEST_IMAGE_TIF)
        self.assertEqual(proc.returncode, 0)
        stderr = proc.stderr.decode("utf-8", errors="replace")
        self.assertIn("image:", stderr)


class TestJbig2ErrorHandling(unittest.TestCase):
    """Error handling for invalid input."""

    def setUp(self):
        _require_jbig2()

    def test_missing_file(self):
        proc = _run("nonexistent.png")
        self.assertNotEqual(proc.returncode, 0)

    def test_invalid_image(self):
        with tempfile.TemporaryDirectory() as tmp:
            bad = Path(tmp) / "bad.txt"
            bad.write_text("not an image")
            proc = _run(str(bad))
            self.assertNotEqual(proc.returncode, 0)


class TestJbig2MultiPage(unittest.TestCase):
    """Multi-page output."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_two_pages_symbol(self):
        """Two images in symbol mode produce multi-page data."""
        proc = _run("-s", TEST_IMAGE_PNG, TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)

    def test_two_pages_generic(self):
        """Generic mode processes two images to stdout."""
        proc = _run(TEST_IMAGE_PNG, TEST_IMAGE_PNG)
        self.assertEqual(proc.returncode, 0)
        self.assertGreater(len(proc.stdout), 0)
        self.assertTrue(proc.stdout.startswith(JBIG2_MAGIC))


class TestJbig2RoundTrip(unittest.TestCase):
    """Round-trip via external decoders when available."""

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PNG)

    def test_roundtrip_jbig2dec(self):
        """Compress then decode with jbig2dec (if available)."""
        jbig2dec = TOOLS.get("jbig2dec")
        if not jbig2dec:
            self.skipTest("jbig2dec not found")
        with tempfile.TemporaryDirectory() as tmp:
            compressed = Path(tmp) / "out.jb2"
            decoded = Path(tmp) / "out.png"
            proc = _run(TEST_IMAGE_PNG)
            self.assertEqual(proc.returncode, 0)
            compressed.write_bytes(proc.stdout)
            r = subprocess.run(
                [str(jbig2dec), "-o", str(decoded), str(compressed)],
                capture_output=True,
                timeout=30,
            )
            self.assertEqual(r.returncode, 0, "jbig2dec decode failed")
            self.assertTrue(decoded.is_file())
            self.assertGreater(decoded.stat().st_size, 0)

    def test_roundtrip_ghostscript(self):
        """Render PDF (via jbig2topdf.py) with Ghostscript (if available)."""
        gs = TOOLS.get("gs") or TOOLS.get("gswin64c")
        if not gs:
            self.skipTest("Ghostscript not found")
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, "-s", "-p", str(TEST_IMAGE_PNG))
            self.assertEqual(proc.returncode, 0)
            pdf_proc = subprocess.run(
                [sys.executable, str(ROOT / "jbig2topdf.py"), "output"],
                capture_output=True,
                timeout=30,
                cwd=tmp,
            )
            self.assertEqual(pdf_proc.returncode, 0)
            pdf = Path(tmp) / "out.pdf"
            pdf.write_bytes(pdf_proc.stdout)
            rendered = Path(tmp) / "rendered.png"
            r = subprocess.run(
                [
                    str(gs),
                    "-dNOPAUSE",
                    "-dBATCH",
                    "-sDEVICE=png16m",
                    f"-sOutputFile={rendered}",
                    "-r72",
                    str(pdf),
                ],
                capture_output=True,
                timeout=60,
            )
            self.assertEqual(r.returncode, 0)
            self.assertTrue(rendered.is_file())
            self.assertGreater(rendered.stat().st_size, 0)

    def test_roundtrip_magick(self):
        """Render PNG (via jbig2topdf.py PDF) with ImageMagick (if available)."""
        magick = TOOLS.get("magick")
        if not magick:
            self.skipTest("ImageMagick not found")
        gs = TOOLS.get("gs") or TOOLS.get("gswin64c")
        if not gs:
            self.skipTest("ImageMagick PDF rendering requires Ghostscript")
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, "-s", "-p", str(TEST_IMAGE_PNG))
            self.assertEqual(proc.returncode, 0)
            pdf_proc = subprocess.run(
                [sys.executable, str(ROOT / "jbig2topdf.py"), "output"],
                capture_output=True,
                timeout=30,
                cwd=tmp,
            )
            self.assertEqual(pdf_proc.returncode, 0)
            pdf = Path(tmp) / "out.pdf"
            pdf.write_bytes(pdf_proc.stdout)
            rendered = Path(tmp) / "rendered.png"
            r = subprocess.run(
                [str(magick), "convert", "-density", "72", str(pdf), str(rendered)],
                capture_output=True,
                timeout=60,
                env=_env(),
            )
            self.assertEqual(r.returncode, 0)
            self.assertTrue(rendered.is_file())
            self.assertGreater(rendered.stat().st_size, 0)


class TestJbig2SegmentPhotoDetection(unittest.TestCase):
    """Regression test for issue #142: photo/graphics detection in -S mode.

    The default local adaptive thresholding path runs
    pixCleanBackgroundToWhite(), which flattens photo/halftone texture so the
    morphology in segment_image() fails to recognise photos as graphics.  The
    fix passes an un-cleaned binary to segment_image() to locate graphics.
    """

    # The regressed build (commit 528a5f4, before the fix) extracted either no
    # graphics at all or ~36 KB from these photo pages; the fixed build extracts
    # ~1.6 MB.  100 KB cleanly separates the two across leptonica versions.
    GRAPHICS_MIN_BYTES = 100_000

    def setUp(self):
        _require_jbig2()
        _require_image(TEST_IMAGE_PHOTO)

    # Expected to fail until the issue #142 photo-detection fix is (re-)applied
    # to jbig2.cc (the un-cleaned mask binary passed to segment_image).  Without
    # that fix the local adaptive path flattens photos and extracts no graphics.
    # Once the fix lands this becomes an "unexpected success" -> remove this
    # decorator.
    @unittest.expectedFailure
    def test_segment_detects_photo(self):
        """-s -S on a photo-heavy image extracts a sizable graphics region."""
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, "-s", "-S", "-b", "out", str(TEST_IMAGE_PHOTO))
            self.assertEqual(proc.returncode, 0, "segment mode failed")
            graphics = Path(tmp) / "out.0000.png"
            self.assertTrue(graphics.is_file(), "graphics output not produced")
            self.assertGreater(
                graphics.stat().st_size,
                self.GRAPHICS_MIN_BYTES,
                "photo region not detected (regression of issue #142)",
            )

    def test_segment_global_mode(self):
        """-s -S -G (global threshold) still detects the photo region."""
        with tempfile.TemporaryDirectory() as tmp:
            proc = _run_cwd(tmp, "-s", "-S", "-G", "-b", "out", str(TEST_IMAGE_PHOTO))
            self.assertEqual(proc.returncode, 0)
            graphics = Path(tmp) / "out.0000.png"
            self.assertTrue(graphics.is_file(), "graphics output not produced")
            self.assertGreater(graphics.stat().st_size, self.GRAPHICS_MIN_BYTES)


if __name__ == "__main__":
    unittest.main()
