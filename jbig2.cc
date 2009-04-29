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

#include <vector>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <allheaders.h>
#include <pix.h>

#include "jbig2enc.h"

static void
usage(const char *argv0) {
  fprintf(stderr, "Usage: %s [options] <input filenames...>\n", argv0);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -d --duplicate-line-removal: use TPGD in generic region coder\n");
  fprintf(stderr, "  -p --pdf: produce PDF ready data\n");
  fprintf(stderr, "  -s --symbol-mode: use text region, not generic coder\n");
  fprintf(stderr, "  -t <threshold>: set classification threshold for symbol coder (def: 0.85)\n");
  fprintf(stderr, "  -T <bw threshold>: set 1 bpp threshold (def: 188)\n");
  fprintf(stderr, "  -r --refine: use refinement (requires -s: lossless)\n");
  fprintf(stderr, "  -O <outfile>: dump thresholded image as PNG\n");
  fprintf(stderr, "  -2: upsample 2x before thresholding\n");
  fprintf(stderr, "  -4: upsample 4x before thresholding\n\n");
}

int
main(int argc, char **argv) {
  bool duplicate_line_removal = false;
  bool pdfmode = false;
  float threshold = 0.85;
  int bw_threshold = 188;
  bool symbol_mode = false;
  bool refine = false;
  bool up2 = false, up4 = false;
  const char *output_threshold = NULL;
  int i;

  for (i = 1; i < argc; ++i) {
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

    if (strcmp(argv[i], "-r") == 0 ||
        strcmp(argv[i], "--refine") == 0) {
      refine = true;
      continue;
    }

    if (strcmp(argv[i], "-2") == 0) {
      up2 = true;
      continue;
    }
    if (strcmp(argv[i], "-4") == 0) {
      up4 = true;
      continue;
    }

    if (strcmp(argv[i], "-O") == 0) {
      output_threshold = argv[i+1];
      i++;
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

    break;
  }

  if (i == argc) {
    fprintf(stderr, "No filename given\n\n");
    usage(argv[0]);
    return 4;
  }

  if (refine && !symbol_mode) {
    fprintf(stderr, "Refinement makes not sense unless in symbol mode!\n");
    fprintf(stderr, "(if you have -r, you must have -s)\n");
    return 5;
  }

  if (up2 && up4) {
    fprintf(stderr, "Can't have both -2 and -4!\n");
    return 6;
  }

  struct jbig2ctx *ctx = jbig2_init(threshold, 0.5, 0, 0, !pdfmode, refine ? 10 : -1);
  const int num_pages = argc - i;

  while (i < argc) {
    PIX *source = pixReadWithHint(argv[i], L_HINT_GRAY);
    if (!source) return 3;

    PIX *pixl, *gray, *pixt;
    if ((pixl = pixRemoveColormap(source, REMOVE_CMAP_TO_GRAYSCALE)) == NULL) {
      fprintf(stderr, "Failed to remove colormap from %s\n", argv[i]);
      return 1;
    }
    pixDestroy(&source);

    if (pixl->d > 1) {
      if (pixl->d > 8) {
        gray = pixConvertRGBToGrayFast(pixl);
        if (!gray) return 1;
      } else {
        gray = pixClone(pixl);
      }
      if (up2) {
        pixt = pixScaleGray2xLIThresh(gray, bw_threshold);
      } else if (up4) {
        pixt = pixScaleGray4xLIThresh(gray, bw_threshold);
      } else {
        pixt = pixThresholdToBinary(gray, bw_threshold);
      }
      pixDestroy(&gray);
    } else {
      pixt = pixClone(pixl);
    }
    pixDestroy(&pixl);

    if (output_threshold) {
      pixWrite(output_threshold, pixt, IFF_PNG);
    }

    if (!symbol_mode) {
      int length;
      uint8_t *ret;
      ret = jbig2_encode_generic(pixt, !pdfmode, 0, 0, duplicate_line_removal,
                                 &length);
      write(1, ret, length);
      return 0;
    }

    jbig2_add_page(ctx, pixt);
    i++;
    pixDestroy(&pixt);
  }

  uint8_t *ret;
  int length;
  ret = jbig2_pages_complete(ctx, &length);
  if (pdfmode) {
    const int fd = open("symboltable", O_WRONLY | O_TRUNC | O_CREAT, 0600);
    if (fd < 0) abort();
    write(fd, ret, length);
    close(fd);
  } else {
    write(1, ret, length);
  }
  free(ret);

  for (int i = 0; i < num_pages; ++i) {
    ret = jbig2_produce_page(ctx, i, -1, -1, &length);
    if (pdfmode) {
      char *filename;
      asprintf(&filename, "page-%d", i);
      const int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
      if (fd < 0) abort();
      write(fd, ret, length);
      close(fd);
      free(filename);
    } else {
      write(1, ret, length);
    }
    free(ret);
  }

  jbig2_destroy(ctx);
}
