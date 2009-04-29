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

#include <map>
#include <algorithm>

#include <allheaders.h>
#include <pix.h>

#include <math.h>
#include <stdint.h>

#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t

#include "jbig2arith.h"
#include "jbig2sym.h"
#include "jbig2structs.h"
#include <netinet/in.h>

#include <sys/time.h>

// -----------------------------------------------------------------------------
// Try each of the nine possible offsets within 1-pixel to see where the
// exemplar of a class fits best. (where best: min number of mismatched
// pixels).
//
// boxes: array of boxes where symbols are
// classa: 2-d array. First dimention is a list of symbols. Then, for each
//         symbol there is a list of all the examples of that symbol. The first
//         example is the exemplar.
// source: source images from which the symbols (as given by boxes) can be
//         taken
// -----------------------------------------------------------------------------
static void
align_boxes(BOXA *boxes, NUMA *assignments, PIXAA *classa, PIX *const source) {
  const int n = boxes->n;

  for (int i = 0; i < n; ++i) {
    const int assigned = (int) assignments->array[i];
    const int boxw = boxes->box[i]->w;
    const int boxh = boxes->box[i]->h;
    const int classw = classa->pixa[assigned]->pix[0]->w;
    const int classh = classa->pixa[assigned]->pix[0]->h;

    const int maxx = std::max(boxw, classw);
    const int maxy = std::max(boxh, classh);
    PIX *back = pixCreate(maxx + 3, maxy + 3, 1);
    if (pixRasterop(back, 1, 1, boxw, boxh,
                PIX_SRC,
                source, boxes->box[i]->x, boxes->box[i]->y)) abort();
    int bestx = 0, besty = 0, bestval = 0xfffffff;
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        PIX *p = pixCopy(NULL, back);
        if (pixRasterop(p, dx + 1, (boxh - classh) + dy + 1, classw, classh,
                    PIX_SRC ^ PIX_DST,
                    classa->pixa[assigned]->pix[0], 0, 0)) abort();
        int val;
        pixCountPixels(p, &val, NULL);
        if (val < bestval) {
          besty = dy;
          bestx = dx;
          bestval = val;
        }
        pixDestroy(&p);
      }
    }

    boxes->box[i]->x += bestx;
    boxes->box[i]->y += besty;
    pixDestroy(&back);
  }
}

// -----------------------------------------------------------------------------
// Removes spots which are less than size x size pixels
//
// Note, this has a side-effect of removing a few pixels
// that from components you want to keep.
//
// If that's a problem, you do a binary reconstruction
// (from seedfill.c):
// -----------------------------------------------------------------------------
static PIX *
remove_flyspecks(PIX *const source, const int size) {
  Sel *sel_5h = selCreateBrick(1, size, 0, 2, SEL_HIT);
  Sel *sel_5v = selCreateBrick(size, 1, 2, 0, SEL_HIT);

  Pix *pixt = pixOpen(NULL, source, sel_5h);
  Pix *pixd = pixOpen(NULL, source, sel_5v);
  pixOr(pixd, pixd, pixt);
  pixDestroy(&pixt);
  selDestroy(&sel_5h);
  selDestroy(&sel_5v);

  return pixd;
}

// -----------------------------------------------------------------------------
// Returns the number of bits needed to encode v symbols
// -----------------------------------------------------------------------------
static unsigned
log2up(int v) {
  unsigned r = 0;
  const bool is_pow_of_2 = (v & (v - 1)) == 0;

  while (v >>= 1) r++;
  if (is_pow_of_2) return r;

  return r + 1;
}

u8 *
jbig2_encode_symbols(struct Pix *const input, const float thresh,
                     const bool full_headers, const int xres,
                     const int yres, int *const length) {
  int segnum = 0;
  *length = 0;

  if (!input) return NULL;
  PIX *bw;
  if (xres >= 300) {
    bw = remove_flyspecks(input, (int) (0.0084*xres));
  } else {
    bw = input;
  }

#ifdef JBIG2_DEBUGGING
  fprintf(stderr, "Symbol coding %d %d\n", bw->w, bw->h);
#endif

  pixSetPadBits(bw, 0);

  PIXA *comps;  // an array of components
  BOXA *boxes = pixConnComp(bw, &comps, 8);
  if (!boxes) return NULL;

  PIXAA *classa;  // 2-d array of classes and examples of that class
  PTA *centroids, *templ_centroids;
  NUMA *classes;  // array of class numbers for each component
  if (jbClassifyCorrel(boxes, comps, thresh, &classa, &centroids,
                          &templ_centroids, &classes)) return NULL;

  align_boxes(boxes, classes, classa, bw);

  struct jbig2_file_header header;
  memset(&header, 0, sizeof(header));
  if (full_headers) {
    header.n_pages = htonl(1);
    header.organisation_type = 1;
    memcpy(&header.id, JBIG2_FILE_MAGIC, 8);
  }

  // setup compression
  struct jbig2enc_ctx ctx;
  jbig2enc_init(&ctx);

  struct jbig2_segment seg;
  memset(&seg, 0, sizeof(seg));
  struct jbig2_segment seg2;
  memset(&seg2, 0, sizeof(seg2));
  struct jbig2_page_info pageinfo;
  memset(&pageinfo, 0, sizeof(pageinfo));
  struct jbig2_text_region textreg;
  memset(&textreg, 0, sizeof(textreg));
  struct jbig2_segment_referring segr;
  memset(&segr, 0, sizeof(segr));
  struct jbig2_symbol_dict symtab;
  memset(&symtab, 0, sizeof(symtab));
  
  seg.number = htonl(segnum++);
  seg.type = segment_page_information;
  seg.page = 1;
  seg.len = htonl(sizeof(struct jbig2_page_info));
  pageinfo.width = htonl(bw->w);
  pageinfo.height = htonl(bw->h);
  pageinfo.xres = htonl(xres ? xres : bw->xres);
  pageinfo.yres = htonl(yres ? yres : bw->yres);
  pageinfo.is_lossless = 0;

  std::map<int, int> symmap;
  jbig2enc_symboltable(&ctx, classa, &symmap);
  const int symdatasize = jbig2enc_datasize(&ctx);
  u8 *const symbuffer = (u8 *) malloc(symdatasize);
  jbig2enc_tobuffer(&ctx, symbuffer);
  jbig2enc_flush(&ctx);

  symtab.a1x = 3;
  symtab.a1y = -1;
  symtab.a2x = -3;
  symtab.a2y = -1;
  symtab.a3x = 2;
  symtab.a3y = -2;
  symtab.a4x = -2;
  symtab.a4y = -2;
  symtab.exsyms = symtab.newsyms = htonl(classa->n);

  const int symtab_segment = segnum;
  seg2.number = htonl(segnum++);
  seg2.type = segment_symbol_table;
  seg2.len = htonl(sizeof(symtab) + symdatasize);
  seg2.page = 1;

  jbig2enc_reset(&ctx);
  const int numsyms = classa->n;
  jbig2enc_textregion(&ctx, symmap, boxes, classa, classes, 1, log2up(numsyms));
  const int textdatasize = jbig2enc_datasize(&ctx);
  u8 *const textbuffer = (u8 *) malloc(textdatasize);
  jbig2enc_tobuffer(&ctx, textbuffer);
  jbig2enc_flush(&ctx);

  textreg.width = htonl(bw->w);
  textreg.height = htonl(bw->h);
  textreg.logsbstrips = 0;
  // refcorner = 0 -> bot left
  textreg.sbnuminstances = htonl(boxes->n);

  segr.number = htonl(segnum++);
  segr.type = segment_imm_text_region;
  segr.len = htonl(sizeof(textreg) + textdatasize);
  segr.referred_segment = symtab_segment;
  segr.segment_count = 1;
  segr.page = 1;

  // find the total size of all the data
  const int totalsize = sizeof(seg) + sizeof(pageinfo) + 
                        sizeof(seg2) + sizeof(symtab) + symdatasize +
                        sizeof(segr) + sizeof(textreg) + textdatasize +
                        (full_headers ? 
                          (sizeof(header) + 2*sizeof(seg)) : 0);

  u8 *const ret = (u8 *) malloc(totalsize);
  int offset = 0;
#define F(x) memcpy(ret + offset, &x, sizeof(x)) ; offset += sizeof(x)
#define G(x, y) memcpy(ret + offset, x, y); offset += y;
  if (full_headers) {
    F(header);
  }
  F(seg);
  F(pageinfo);
  F(seg2);
  F(symtab);
  G(symbuffer, symdatasize);
  F(segr);
  F(textreg);
  G(textbuffer, textdatasize);

  free(symbuffer);
  free(textbuffer);
  
  if (full_headers) {
    seg.type = segment_end_of_page;
    seg.len = 0;
    F(seg);
    seg.type = segment_end_of_file;
    F(seg);
  }

#undef F
#undef G

  if (totalsize != offset) abort();

  jbig2enc_dealloc(&ctx);
  pixaDestroy(&comps);
  boxaDestroy(&boxes);
  pixaaDestroy(&classa);
  numaDestroy(&classes);
  ptaDestroy(&centroids);
  ptaDestroy(&templ_centroids);
  if (bw != input) pixDestroy(&bw);

  *length = offset;
  return ret;
}

u8 *
jbig2_encode_generic(struct Pix *const bw, const bool full_headers, const int xres,
                     const int yres, const bool duplicate_line_removal,
                     int *const length) {
  int segnum = 0;

  if (!bw) return NULL;
  pixSetPadBits(bw, 0);

  struct jbig2_file_header header;
  if (full_headers) {
    memset(&header, 0, sizeof(header));
    header.n_pages = htonl(1);
    header.organisation_type = 1;
    memcpy(&header.id, JBIG2_FILE_MAGIC, 8);
  }

  // setup compression
  struct jbig2enc_ctx ctx;
  jbig2enc_init(&ctx);

  jbig2_segment seg;
  memset(&seg, 0, sizeof(seg));
  jbig2_segment seg2;
  memset(&seg2, 0, sizeof(seg2));
  jbig2_page_info pageinfo;
  memset(&pageinfo, 0, sizeof(pageinfo));
  jbig2_generic_region genreg;
  memset(&genreg, 0, sizeof(genreg));

  seg.number = htonl(segnum++);
  seg.type = segment_page_information;
  seg.page = 1;
  seg.len = htonl(sizeof(struct jbig2_page_info));
  pageinfo.width = htonl(bw->w);
  pageinfo.height = htonl(bw->h);
  pageinfo.xres = htonl(xres ? xres : bw->xres);
  pageinfo.yres = htonl(yres ? yres : bw->yres);
  pageinfo.is_lossless = 1;
  
  jbig2enc_bitimage(&ctx, (u8 *) bw->data, bw->w, bw->h, duplicate_line_removal);
  jbig2enc_final(&ctx);
  const int datasize = jbig2enc_datasize(&ctx);

  seg2.number = htonl(segnum++);
  seg2.type = segment_imm_generic_region;
  seg2.page = 1;
  seg2.len = htonl(sizeof(genreg) + datasize);

  genreg.width = htonl(bw->w);
  genreg.height = htonl(bw->h);
  if (duplicate_line_removal) {
    genreg.tpgdon = true;
  }
  genreg.a1x = 3;
  genreg.a1y = -1;
  genreg.a2x = -3;
  genreg.a2y = -1;
  genreg.a3x = 2;
  genreg.a3y = -2;
  genreg.a4x = -2;
  genreg.a4y = -2;
  
  const int totalsize = sizeof(seg) + sizeof(pageinfo) + sizeof(seg2) +
                        sizeof(genreg) + datasize +
                        (full_headers ? (sizeof(header) + 2*sizeof(seg)) : 0);
  u8 *const ret = (u8 *) malloc(totalsize);
  int offset = 0;
  
#define F(x) memcpy(ret + offset, &x, sizeof(x)) ; offset += sizeof(x)
  if (full_headers) {
    F(header);
  }
  F(seg);
  F(pageinfo);
  F(seg2);
  F(genreg);
  jbig2enc_tobuffer(&ctx, ret + offset);
  offset += datasize;
  
  if (full_headers) {
    seg.type = segment_end_of_page;
    seg.len = 0;
    F(seg);
    seg.type = segment_end_of_file;
    F(seg);
  }

  if (totalsize != offset) abort();

  jbig2enc_dealloc(&ctx);

  *length = offset;

  return ret;
}
