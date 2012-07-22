// Copyright 2006 Google Inc. All Rights Reserved.
// Author: hata.radim@gmail.com (Radim Hatlapatka)
//
// Copyright (C) 2012 Google Inc.
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

#ifndef JBIG2COMPARATOR_H__
#define JBIG2COMPARATOR_H__

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

#if defined(sun)
#include <sys/types.h>
#else
#include <stdint.h>
#endif

#include <leptonica/allheaders.h>

struct Pix;

/**
 * Compares two pix and tell if they are equivalent by trying to decide if these symbols are equivalent from visual
 * point of view
 *
 * It works by looking for accumulations of differences between two templates
 *
 * If the difference is bigger than concrete percentage of one of templates they are considered different, if such
 * difference doesn't exist than they are equivalent
 */
int are_equivalent(PIX *const firstTemplate, PIX *const secondTemplate);

/**
 * Printing PIX as bitmap to standard error output
 */
void print_pix(PIX *pix);

#endif  // JBIG2OCR_JBIG2_H__
