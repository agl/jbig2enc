[![autotools](https://github.com/agl/jbig2enc/actions/workflows/autotools.yaml/badge.svg)](https://github.com/agl/jbig2enc/actions/workflows/autotools.yaml) [![Msys2](https://github.com/agl/jbig2enc/actions/workflows/msys2.yaml/badge.svg)](https://github.com/agl/jbig2enc/actions/workflows/msys2.yaml)

This is an encoder for [JBIG2](doc/fcd14492.pdf).

JBIG2 encodes bi-level (1 bpp) images using a number of clever tricks to get
better compression than G4. This encoder can:
   * Generate JBIG2 files, or fragments for embedding in PDFs
   * Generic region encoding
   * Perform symbol extraction, classification and text region coding
   * Perform refinement coding and,
   * Compress multipage documents

It uses the (Apache-ish licensed) Leptonica library:
  http://leptonica.com/

You'll need version 1.74.

## Known bugs

The refinement coding causes Acrobat to crash. It's not known if this is a bug
in Acrobat, though it may well be.


## Usage

_Note_: Windows Command Prompt does not support wildcard expansion, so `*.jpg` will not work. You'll need to manually expand the file names yourself or you need to use the latest git code and [MSVC build](https://learn.microsoft.com/en-us/cpp/c-language/expanding-wildcard-arguments).

See the `jbig2enc.h` header for the high level API, or the `jbig2` program for an
example of usage:

```
$ jbig2 -s -a -p -v *.jpg && python3 jbig2topdf.py output >out.pdf
```

or with standalone mode:

```
$ jbig2 -a -p -v images/feyn.tif > feyn.jb2 && python3 jbig2topdf.py -s feyn.jb2 > feyn.pdf
```

to encode jbig2 files for pdf creation.
If you want to encode an image and then view output first to include in pdf

```
$ jbig2 -s -S -p -v -O out.png *.jpg
```

If you want to encode an image as jbig2 (can be view in [STDU Viewer](http://www.stdutility.com/stduviewer.html) on Windows) run:

```
$ jbig2 -s images/feyn.tif >feyn.jb2
```

### Links:

* [jbig2enc-samples](https://github.com/zdenop/jbig2enc-samples)
* [jbig2enc-minidjvu](https://github.com/ImageProcessing-ElectronicPublications/jbig2enc-minidjvu)
