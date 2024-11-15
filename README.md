This is an encoder for [JBIG2](fcd14492.pdf).

JBIG2 encodes bi-level (1 bpp) images using a number of clever tricks to get
better compression than G4. This encoder can:
   * Generate JBIG2 files, or fragments for embedding in PDFs
   * Generic region encoding
   * Perform symbol extraction, classification and text region coding
   * Perform refinement coding and,
   * Compress multipage documents

It uses the (Apache-ish licensed) Leptonica library:
  http://leptonica.com/

You'll need version 1.68.

## Known bugs

The refinement coding causes Acrobat to crash. It's not known if this is a bug
in Acrobat, though it may well be.


## Usage

See the `jbig2enc.h` header for the high level API, or the `jbig2` program for an
example of usage:

```
$ jbig2 -s -p -v *.jpg && python3 jbig2topdf.py output >out.pdf
```

or with standalone mode:

```
$ jbig2 -p -v images/feyn.tif > feyn.jbig2 && python3 jbig2topdf.py -s feyn.jbig2 > feyn.pdf
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


# Building

## Prerequisites

* installed [leptonica](http://www.leptonica.org/) including development parts
* installed [cmake](https://cmake.org/) or [autotools] (https://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html)
* installed C++ compiller (gcc, clang, MSVC)
* installed [git](https://git-scm.com/)


## Cmake

### Windows


*Note*: `cat`, `rm` and `dos2unix` tool are part of [git for windows](https://gitforwindows.org/). You can add them to your path with `set PATH=%PATH%;C:\Program Files\Git\usr\bin`. Adjust path `f:\win64` to your leptonica installation.

```
"c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" x64
set INSTALL_DIR=f:\win64
set INCLUDE_DIR=f:\win64\include
set LIB_DIR=f:\win64\lib
set PATH=%PATH%;%INSTALL_DIR%\bin
```

### Configuration

```
git clone --depth 1 https://github.com/agl/jbig2enc
cmake -Bbuild -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DCMAKE_PREFIX_PATH=%INSTALL_DIR%
cmake --build build --config Release
```

### Install

```
cmake --build build --config Release --target install
```

### Uninstall

```
cat build/install_manifest.txt | dos2unix | xargs rm

```

### Clean

```
rm -r build/*
```
