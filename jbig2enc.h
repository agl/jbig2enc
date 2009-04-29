// Copyright 2006 Google Inc. All Rights Reserved.
// Author: agl@imperialviolet.org (Adam Langley)
//
// Copyright (C) 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef JBIG2ENC_JBIG2_H__
#define JBIG2ENC_JBIG2_H__

// -----------------------------------------------------------------------------
// Welcome gentle reader,
//
// This is an encoder for JBIG2:
// www.jpeg.org/public/fcd14492.pdf
//
// JBIG2 encodes bi-level (1 bpp) images using a number of clever tricks to get
// better compression than G4. This encoder can:
//    * Generate JBIG2 files, or fragments for embedding in PDFs
//    * Generic region encoding
//    * Symbol extraction, classification and text region coding
//
// It uses the (Apache-ish licensed) Leptonica library:
//   http://www.leptonica.com/
// -----------------------------------------------------------------------------

#include <stdint.h>

struct Pix;

// -----------------------------------------------------------------------------
// Encode an image by finding the connected components, grouping them together
// and encoding the positions of them on the page. This is clearly designed for
// scanned text pages and is *lossy*.
//
// bw: A 1-bpp image
// thresh: The threshold for the classifier. The larger the number the larger
//         the number of different symbols, the more bits used and the closer
//         the resulting image is to the original. (0.85 is a good value)
// full_headers: if true a full JBIG2 file is produced, otherwise the data is
//               only good for enbedding
// xres: the ppi in the X direction. If 0, the ppi is taken from bw
// yres: see xres
// length: the length of the returned data is written here
//
// WARNING: returns a malloced buffer which the caller must free
// -----------------------------------------------------------------------------
uint8_t *
jbig2_encode_symbols(struct Pix *const bw, const float thresh,
                     const bool full_headers, const int xres,
                     const int yres, int *length);


// -----------------------------------------------------------------------------
// Encode an image as a single generic region. This is lossless. It should not
// be used for images as half-tone coding is not implemented.
//
// see argument comments for jbig2_encode_symbols
// duplicate_line_removal: turning this on
//    * Breaks ghostscript
//    * Takes ever so slightly more bytes to encode
//    * Cuts the encode time by half
//
// WARNING: returns a malloced buffer which the caller must free
// -----------------------------------------------------------------------------
uint8_t *
jbig2_encode_generic(struct Pix *const bw, const bool full_headers,
                     const int xres, const int yres,
                     const bool duplicate_line_removal,
                     int *const length);

#endif  // JBIG2ENC_JBIG2_H__
