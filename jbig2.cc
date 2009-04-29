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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <allheaders.h>
#include <pix.h>

#include "jbig2enc.h"

static void
usage(const char *argv0) {
  fprintf(stderr, "Usage: %s [options] <input filename>\n", argv0);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -d --duplicate-line-removal: use TPGD in generic region coder\n");
  fprintf(stderr, "  -p --pdf: produce PDF ready data\n");
  fprintf(stderr, "  -s --symbol-mode: use text region, not generic coder\n");
  fprintf(stderr, "  -t <threshold>: set classification threshold for symbol coder (def: 0.85)\n");
  fprintf(stderr, "  -T <bw threshold>: set 1 bpp threshold (def: 188)\n\n");
}

int
main(int argc, char **argv) {
  bool duplicate_line_removal = false;
  bool pdfmode = false;
  float threshold = 0.85;
  int bw_threshold = 188;
  bool symbol_mode = false;
  const char *filename = NULL;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-d") == 0 ||
        strcmp(argv[i], "--duplicate-line-removal") == 0) {
      duplicate_line_removal = true;
      continue;
    }

    if (strcmp(argv[i], "-p") == 0 ||
        strcmp(argv[i], "--pdf") == 0) {
      pdfmode = true;
      continue;
    }

    if (strcmp(argv[i], "-s") == 0 ||
        strcmp(argv[i], "--symbol-mode") == 0) {
      symbol_mode = true;
      continue;
    }

    if (strcmp(argv[i], "-t") == 0) {
      char *endptr;
      threshold = strtod(argv[i+1], &endptr);
      if (*endptr) {
        fprintf(stderr, "Cannot parse float value: %s\n", argv[i+1]);
        usage(argv[0]);
        return 1;
      }

      if (threshold > 1.0 | threshold < 0.0) {
        fprintf(stderr, "Invalid value for threshold\n");
        return 10;
      }
      i++;
      continue;
    }

    if (strcmp(argv[i], "-T") == 0) {
      char *endptr;
      bw_threshold = strtol(argv[i+1], &endptr, 10);
      if (*endptr) {
        fprintf(stderr, "Cannot parse int value: %s\n", argv[i+1]);
        usage(argv[0]);
        return 1;
      }
      if (bw_threshold < 0 || bw_threshold > 255) {
        fprintf(stderr, "Invalid bw threshold: (0..255)\n");
        return 11;
      }
      i++;
      continue;
    }

    if (filename) {
      fprintf(stderr, "Unknown arg: %s\n", argv[i]);
      usage(argv[0]);
      return 2;
    }

    filename = argv[i];
  }

  if (!filename) {
    fprintf(stderr, "No filename given\n\n");
    usage(argv[0]);
    return 4;
  }

  PIX *source = pixRead(filename);
  if (!source) return 3;

  PIX *pixt;
  if ((pixt = pixRemoveColormap(source, REMOVE_CMAP_TO_GRAYSCALE)) == NULL) {
    return 1;
  }

  if (pixt->d > 1) {
    PIX *gray;
    if (pixt->d > 8) {
      gray = pixConvertRGBToGray(pixt, 0.0, 0.0, 0.0);
      if (!gray) return 1;
    } else {
      gray = pixt;
    }
    pixt = pixThresholdToBinary(gray, bw_threshold);
  }
  uint8_t *ret;
  int length;

  if (symbol_mode) {
    ret = jbig2_encode_symbols(pixt, threshold, !pdfmode, 0, 0, &length);
  } else {
    ret = jbig2_encode_generic(pixt, !pdfmode, 0, 0, duplicate_line_removal,
                               &length);
  }

  if (!ret) {
    fprintf(stderr, "Error in coding\n");
    return 12;
  }

  write(1, ret, length);
  free(ret);
}
