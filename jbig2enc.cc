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
#include <vector>
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

// -----------------------------------------------------------------------------
// This is the context for a multi-page JBIG2 document.
// -----------------------------------------------------------------------------
struct jbig2ctx {
  struct JbClasser *classer;  // the leptonica classifier
  int xres, yres;  // ppi for the X and Y direction
  bool full_headers;  // true if we are producing a full JBIG2 file
  bool pdf_page_numbering;  // true if all text pages are page "1" (pdf mode)
  int segnum;  // current segment number
  int symtab_segment;  // the segment number of the symbol table
  // a map from page number a list of components for that page
  std::map<int, std::vector<int> > pagecomps;
  std::vector<int> page_width, page_height;
  std::map<int, int> symmap;
  bool refinement;
  int refine_level;
  // only used when using refinement
    // the bounding boxes of the symbols of each page.
    std::vector<BOXA *> boxes;
    // the number of the first symbol of each page
    std::vector<int> baseindexes;
    std::vector<PIXA *> comps;
};

// see comments in .h file
struct jbig2ctx *
jbig2_init(float thresh, float weight, int xres, int yres, bool full_headers,
           int refine_level) {
  struct jbig2ctx *ctx = new jbig2ctx;
  ctx->xres = xres;
  ctx->yres = yres;
  ctx->full_headers = full_headers;
  ctx->pdf_page_numbering = !full_headers;
  ctx->segnum = 0;
  ctx->symtab_segment = -1;
  ctx->refinement = refine_level >= 0;
  ctx->refine_level = refine_level;

  ctx->classer = jbCorrelationInit(JB_CONN_COMPS, 9999, 9999, thresh, weight);

  return ctx;
}

// see comments in .h file
void
jbig2_destroy(struct jbig2ctx *ctx) {
  for (std::vector<BOXA *>::iterator i = ctx->boxes.begin();
       i != ctx->boxes.end(); ++i) {
    boxaDestroy(&(*i));
  }
  for (std::vector<PIXA *>::iterator i = ctx->comps.begin();
       i != ctx->comps.end(); ++i) {
    pixaDestroy(&(*i));
  }
  jbClasserDestroy(&ctx->classer);
  delete ctx;
}

// see comments in .h file
void
jbig2_add_page(struct jbig2ctx *ctx, struct Pix *input) {
  PIX *bw;
  if (ctx->xres >= 300) {
    bw = remove_flyspecks(input, (int) (0.0084*ctx->xres));
  } else {
    bw = input;
  }
  if (ctx->refinement) {
    ctx->baseindexes.push_back(ctx->classer->baseindex);
  }

  jbAddPage(ctx->classer, bw);
  ctx->page_width.push_back(bw->w);
  ctx->page_height.push_back(bw->h);

  if (ctx->refinement) {
    BOXA *boxes = boxaCopy(ctx->classer->boxas, L_CLONE);
    ctx->boxes.push_back(boxes);
    PIXA *comps = pixaCopy(ctx->classer->pixas, L_CLONE);
    ctx->comps.push_back(comps);
  }

  if (bw != input) {
    pixDestroy(&bw);
  }
}

#define F(x) memcpy(ret + offset, &x, sizeof(x)) ; offset += sizeof(x)
#define G(x, y) memcpy(ret + offset, x, y); offset += y;

// see comments in .h file
uint8_t *
jbig2_pages_complete(struct jbig2ctx *ctx, int *const length) {
  // our first task is to build the pagecomps map: a map from page number to
  // the list of connected components for that page.
  // the classer gives us an array from connected component number to page
  // number - we just have to reverse it
  for (int i = 0; i < ctx->classer->napage->n; ++i) {
    int page_num;
    numaGetIValue(ctx->classer->napage, i, &page_num);
    ctx->pagecomps[page_num].push_back(i);
  }

#ifdef SYMBOL_COMPRESSION_DEBUGGING
  std::map<int, int> usecount;
  for (int i = 0; i < ctx->classer->naclass->n; ++i) {
    usecount[(int)ctx->classer->naclass->array[i]]++;
  }

  for (int p = 0; p < ctx->classer->npages; ++p) {
    const int numcomps = ctx->pagecomps[p].size();
    int unique_in_doc = 0;
    std::map<int, int> symcount;
    for (std::vector<int>::const_iterator i = ctx->pagecomps[p].begin();
         i != ctx->pagecomps[p].end(); ++i) {
      const int sym = (int) ctx->classer->naclass->array[*i];
      symcount[sym]++;
      if (usecount[sym] == 1) unique_in_doc++;
    }
    int unique_this_page = 0;
    for (std::map<int, int>::const_iterator i = symcount.begin();
         i != symcount.end(); ++i) {
      if (i->second == 1) unique_this_page++;
    }

    fprintf(stderr, "Page %d %d/%d/%d\n", p, numcomps, unique_this_page, unique_in_doc);
  }
#endif

  jbGetLLCorners(ctx->classer);

  struct jbig2enc_ctx ectx;
  jbig2enc_init(&ectx);

  struct jbig2_file_header header;
  if (ctx->full_headers) {
    memset(&header, 0, sizeof(header));
    header.n_pages = htonl(ctx->classer->npages);
    header.organisation_type = 1;
    memcpy(&header.id, JBIG2_FILE_MAGIC, 8);
  }

  struct jbig2_segment seg;
  memset(&seg, 0, sizeof(seg));
  struct jbig2_symbol_dict symtab;
  memset(&symtab, 0, sizeof(symtab));

  jbig2enc_symboltable(&ectx, ctx->classer->pixat, &ctx->symmap);
  const int symdatasize = jbig2enc_datasize(&ectx);

  symtab.a1x = 3;
  symtab.a1y = -1;
  symtab.a2x = -3;
  symtab.a2y = -1;
  symtab.a3x = 2;
  symtab.a3y = -2;
  symtab.a4x = -2;
  symtab.a4y = -2;
  symtab.exsyms = symtab.newsyms = htonl(ctx->classer->pixat->n);

  ctx->symtab_segment = ctx->segnum;
  seg.number = htonl(ctx->segnum);
  ctx->segnum++;
  seg.type = segment_symbol_table;
  seg.len = htonl(sizeof(symtab) + symdatasize);
  seg.page = 0;
  seg.retain_bits = 1;

  u8 *const ret = (u8 *) malloc((ctx->full_headers ? sizeof(header) : 0) +
                                sizeof(seg) + sizeof(symtab) + symdatasize);
  int offset = 0;
  if (ctx->full_headers) {
    F(header);
  }
  F(seg);
  F(symtab);
  jbig2enc_tobuffer(&ectx, ret + offset);
  jbig2enc_dealloc(&ectx);
  offset += symdatasize;

  *length = offset;

  return ret;
}

// see comments in .h file
uint8_t *
jbig2_produce_page(struct jbig2ctx *ctx, int page_no,
                   int xres, int yres, int *const length) {
  const bool last_page = page_no == ctx->classer->npages;
  const bool include_trailer = last_page && ctx->full_headers;

  struct jbig2enc_ctx ectx;
  jbig2enc_init(&ectx);

  struct jbig2_segment seg;
  memset(&seg, 0, sizeof(seg));
  struct jbig2_segment endseg;
  memset(&endseg, 0, sizeof(endseg));
  struct jbig2_page_info pageinfo;
  memset(&pageinfo, 0, sizeof(pageinfo));
  struct jbig2_text_region textreg;
  memset(&textreg, 0, sizeof(textreg));
  struct jbig2_text_region_syminsts textreg_syminsts;
  memset(&textreg_syminsts, 0, sizeof(textreg_syminsts));
  struct jbig2_text_region_atflags textreg_atflags;
  memset(&textreg_atflags, 0, sizeof(textreg_atflags));
  struct jbig2_segment_referring segr;
  memset(&segr, 0, sizeof(segr));
  struct jbig2_segment_referring_trailer segrt;
  memset(&segrt, 0, sizeof(segrt));

  seg.number = htonl(ctx->segnum);
  ctx->segnum++;
  seg.type = segment_page_information;
  seg.page = ctx->pdf_page_numbering ? 1 : 1 + page_no;;
  seg.len = htonl(sizeof(struct jbig2_page_info));
  pageinfo.width = htonl(ctx->page_width[page_no]);
  pageinfo.height = htonl(ctx->page_height[page_no]);
  pageinfo.xres = xres == -1 ? ctx->xres : xres;
  pageinfo.yres = yres == -1 ? ctx->yres : yres;
  pageinfo.is_lossless = ctx->refinement;

  const int numsyms = ctx->classer->pixat->n;
  BOXA *const boxes = ctx->refinement ? ctx->boxes[page_no] : NULL;
  int baseindex = ctx->refinement ? ctx->baseindexes[page_no] : 0;
  jbig2enc_textregion(&ectx, ctx->symmap, ctx->pagecomps[page_no],
                      ctx->classer->ptall, ctx->classer->pixat,
                      ctx->classer->naclass, 1,
                      log2up(numsyms),
                      ctx->refinement ? ctx->comps[page_no] : NULL,
                      boxes, baseindex, ctx->refine_level);
  const int textdatasize = jbig2enc_datasize(&ectx);
  textreg.width = htonl(ctx->page_width[page_no]);
  textreg.height = htonl(ctx->page_height[page_no]);
  textreg.logsbstrips = 0;
  textreg.sbrefine = ctx->refinement;
  // refcorner = 0 -> bot left
  textreg_syminsts.sbnuminstances = htonl(ctx->pagecomps[page_no].size());

  textreg_atflags.a1x = -1;
  textreg_atflags.a1y = -1;
  textreg_atflags.a2x = -1;
  textreg_atflags.a2y = -1;

  const u32 this_segment_number = ctx->segnum;
  segr.number = htonl(ctx->segnum);
  ctx->segnum++;
  segr.type = segment_imm_text_region;
  if (ctx->refinement) {
        segrt.len = htonl(sizeof(textreg) + sizeof(textreg_syminsts) +
                     sizeof(textreg_atflags) + textdatasize);
  } else {
    segrt.len = htonl(sizeof(textreg) + sizeof(textreg_syminsts) + textdatasize);
  }

  // Segments can only refer to previous segments. So the size of the referred
  // to segments depends on the number of *this* segments. see 7.2.5 pp 76)
  int referring_data_len;
  u8 referring_data[4];
  if (this_segment_number <= 256) {
    referring_data_len = 1;
    referring_data[0] = ctx->symtab_segment;
  } else if (this_segment_number <= 65536) {
    referring_data_len = 2;
    u16 r = htons(ctx->symtab_segment);
    memcpy(referring_data, &r, 2);
  } else {
    referring_data_len = 4;
    u32 r = htonl(ctx->symtab_segment);
    memcpy(referring_data, &r, 4);
  }

  segr.segment_count = 1;
  segr.retain_bits = 2;
  segrt.page = ctx->pdf_page_numbering ? 1 : 1 + page_no;;

  const int totalsize = sizeof(seg) + sizeof(pageinfo) + sizeof(segr) +
                        referring_data_len + sizeof(segrt) +
                        sizeof(textreg) + sizeof(textreg_syminsts) +
                        (ctx->refinement ? sizeof(textreg_atflags) : 0) +
                        textdatasize +
                        (ctx->full_headers ? sizeof(endseg) : 0) +
                        (include_trailer ? sizeof(seg) : 0);
  u8 *ret = (u8 *) malloc(totalsize);
  int offset = 0;

  F(seg);
  F(pageinfo);
  F(segr);
  G(referring_data, referring_data_len);
  F(segrt);
  F(textreg);
  if (ctx->refinement) {
    F(textreg_atflags);
  }
  F(textreg_syminsts);
  jbig2enc_tobuffer(&ectx, ret + offset); offset += textdatasize;
  if (ctx->full_headers) {
    endseg.number = htonl(ctx->segnum);
    ctx->segnum++;
    endseg.type = segment_end_of_page;
    endseg.page = ctx->pdf_page_numbering ? 1 : 1 + page_no;
    F(endseg);
  }
  if (include_trailer) {
    endseg.number = htonl(ctx->segnum);
    ctx->segnum++;
    endseg.type = segment_end_of_file;
    endseg.page = 0;
    F(endseg);
  }

  if (totalsize != offset) abort();

  jbig2enc_dealloc(&ectx);

  *length = offset;
  return ret;
}

#undef F
#undef G

// see comments in .h file
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

  seg.number = htonl(segnum);
  segnum++;
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

  seg2.number = htonl(segnum);
  segnum++;
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

