# JBIG2 Encoder

[![autotools](https://github.com/agl/jbig2enc/actions/workflows/autotools.yaml/badge.svg)](https://github.com/agl/jbig2enc/actions/workflows/autotools.yaml) [![Msys2](https://github.com/agl/jbig2enc/actions/workflows/msys2.yaml/badge.svg)](https://github.com/agl/jbig2enc/actions/workflows/msys2.yaml) [![meson](https://github.com/agl/jbig2enc/actions/workflows/meson.yaml/badge.svg)](https://github.com/agl/jbig2enc/actions/workflows/meson.yaml) [![cmake](https://github.com/agl/jbig2enc/actions/workflows/cmake.yaml/badge.svg)](https://github.com/agl/jbig2enc/actions/workflows/cmake.yaml)

This is an encoder for [JBIG2](doc/fcd14492.pdf).

JBIG2 encodes bi-level (1 bpp) images using a number of clever tricks to get
better compression than G4. This encoder can:

* Generate JBIG2 files, or fragments for embedding in PDFs
* Generic region encoding
* Perform symbol extraction, classification and text region coding
* Perform refinement coding and,
* Compress multipage documents

It uses the (Apache-ish licensed) [Leptonica library](https://github.com/danbloomberg/leptonica). Version 1.74 or later is required.

## Known bugs

The refinement coding causes Acrobat to crash. It's not known if this is a bug
in Acrobat, though it may well be.

## Usage

_Note_: Windows Command Prompt does not support wildcard expansion, so `*.jpg` will not work. You'll need to manually expand the file names yourself or you need to use the latest git code and [MSVC build](https://learn.microsoft.com/en-us/cpp/c-language/expanding-wildcard-arguments).

See the `jbig2enc.h` header for the high level API, or the `jbig2` program for an
example of usage:

```sh
jbig2 -s -a -p -v -T 100 -b imgs images/*.jpg && python jbig2topdf.py imgs >out.pdf
```

or with standalone mode:

```sh
jbig2 -a -p -v images/feyn.tif > feyn.jb2 && python jbig2topdf.py -s feyn.jb2 > feyn.pdf
```

to encode jbig2 files for pdf creation.
If you want to encode an image and then view output first to include in pdf

```sh
jbig2 -s -S -p -v -O out.png images/*.jpg
```

If you want to encode an image as jbig2 (can be view in [STDU Viewer](http://www.stdutility.com/stduviewer.html) on Windows) run:

```sh
jbig2 -s images/feyn.tif >feyn.jb2
```

### Glyph provenance sidecar (`--emit-json`)

In symbol mode the classifier assigns every component instance to a symbol
class and records where the class template is placed. That information is
consumed into the arithmetic-coded text regions and normally lost to callers.
With `--emit-json` (requires `-s`) it is written to a JSON sidecar, so the
encoder can double as a glyph clustering layer for downstream tools:

```sh
jbig2 -s -p -b output --emit-json output.json images/amoris.2.150.jpg
```

This writes the usual symbol mode outputs (`output.sym`, `output.0000`) plus
`output.json` with the provenance of every instance:

```json
{
  "version": 1,
  "num_pages": 1,
  "num_symbols": 929,
  "num_instances": 3089,
  "pages": [
    {"page": 1, "width": 855, "height": 1263, "xres": 150, "yres": 150}
  ],
  "symbols": [
    {"class": 0, "width": 18, "height": 264}
  ],
  "instances": [
    {"class": 0, "page": 1, "ul": [-1, -1], "ll": [0, 263]}
  ]
}
```

All coordinates are raster coordinates of the page at encode resolution: `x`
is measured from the left edge, `y` from the top edge. For each instance,
`ul` is the upper left corner of the template placement box and `ll` the
lower left corner of the ink, which is the placement used by the JBIG2 text
region coder. Both corners come from the classifier's placement arrays, and
for a component touching the top or left crop edge the anchor can fall a
pixel outside the page raster, so small negative values as in the sample are
expected and not an error. `symbols` holds one entry per class with the exemplar
dimensions in pixels. Pages that were not encoded in symbol mode report a
zero instance count. The flag is default-off and writes only to the named
file, leaving the stdout/stderr contract unchanged.

### Links

* [jbig2enc-samples](https://github.com/zdenop/jbig2enc-samples)
* [jbig2enc-minidjvu](https://github.com/ImageProcessing-ElectronicPublications/jbig2enc-minidjvu)
