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
#ifdef USE_EXT
#include <ext/numeric>  // for iota
#else
#include <numeric>  // for iota
#endif

#include "jbig2arith.h"

#define restrict __restrict__

#include <allheaders.h>
#include <pix.h>

#define S(i) symbols->pixa[i]->pix[0]

// -----------------------------------------------------------------------------
// Sorts a vector of indexes into the symbols PIXAA by height. This is needed
// because symbols are placed into the JBIG2 table in height order
// -----------------------------------------------------------------------------
class HeightSorter {  // concept: stl/StrictWeakOrdering
 public:
  HeightSorter(const PIXAA *isymbols)
      : symbols(isymbols) {}

  bool operator() (int x, int y) {
    return S(x)->h < S(y)->h;
  }

 private:
  const PIXAA *const symbols;
};

// -----------------------------------------------------------------------------
// Sorts a vector of indexes into the symbols PIXAA by width. This is needed
// because symbols are placed into the JBIG2 table in width order (for a given
// height class)
// -----------------------------------------------------------------------------
class WidthSorter {  // concept: stl/StrictWeakOrdering
 public:
  WidthSorter(const PIXAA *isymbols)
      : symbols(isymbols) {}

  bool operator() (int x, int y) {
    return S(x)->w < S(y)->w;
  }

 private:
  const PIXAA *const symbols;
};

// see comment in .h file
void
jbig2enc_symboltable(struct jbig2enc_ctx *restrict ctx,
                     PIXAA *restrict const symbols, std::map<int, int> *symmap) {
  const unsigned n = symbols->n;
  int number = 0;

#ifdef JBIG2_DEBUGGING
  fprintf(stderr, "  symbols: %d\n", n);
#endif

  // this is a vector of indexed into symbols
  std::vector<int> syms(n);
  // fill the vector with the index of each symbol
  iota(syms.begin(), syms.end(), 0);
  // now sort that vector by height
  std::sort(syms.begin(), syms.end(), HeightSorter(symbols));

  // this is used for each height class to sort into increasing width
  WidthSorter sorter(symbols);

  // this stores the indexes of the symbols for a given height class
  std::vector<int> hc;
  // this keeps the value of the height of the current class
  unsigned hcheight = 0;
  for (unsigned i = 0; i < n;) {
    const unsigned height = S(syms[i])->h;  // height of this class
    //fprintf(stderr, "height is %d\n", height);
    unsigned j;
    hc.clear();
    hc.push_back(syms[i]);  // this is the first member of the new class
    // walk the vector until we find a symbol with a different height
    for (j = i + 1; j < n; ++j) {
      if (S(syms[j])->h != height) break;
      hc.push_back(syms[j]);  // add each symbol of the same height to the class
    }
#ifdef JBIG2_DEBUGGING
    fprintf(stderr, "  hc (height: %d, members: %d)\n", height, hc.size());
#endif
    // all the symbols from i to j-1 are a height class
    // now sort them into increasing width
    sort(hc.begin(), hc.end(), sorter);
    // encode the delta height
    const int deltaheight = height - hcheight;
    jbig2enc_int(ctx, JBIG2_IADH, deltaheight);
    hcheight = height;
    int symwidth = 0;
    // encode each symbol
    for (std::vector<int>::const_iterator k = hc.begin(); k != hc.end(); ++k) {
      const int sym = *k;
      const int deltawidth = S(sym)->w - symwidth;
#ifdef JBIG2_DEBUGGING
      fprintf(stderr, "    h: %d\n", S(sym)->w);
#endif
      symwidth += deltawidth;
      //fprintf(stderr, "width is %d\n", S(sym)->w);
      jbig2enc_int(ctx, JBIG2_IADW, deltawidth);
      // encoding the bitmap requires that the pad bits be zero
      pixSetPadBits(S(sym), 0);
      jbig2enc_bitimage(ctx, (uint8_t *) S(sym)->data, S(sym)->w, height,
                        false);
      // add this symbol to the map
      (*symmap)[sym] = number++;
    }
    // OOB marks the end of the height class
    //fprintf(stderr, "OOB\n");
    jbig2enc_oob(ctx, JBIG2_IADW);
    i = j;
  }

  // now we have the list of exported symbols (which is all of them)
  // it's run length encoded and we have a run length of 0 (for all the symbols
  // which aren't set) followed by a run length of the number of symbols
  jbig2enc_int(ctx, JBIG2_IAEX, 0);
  jbig2enc_int(ctx, JBIG2_IAEX, n);

  jbig2enc_final(ctx);
}

#define BY(a) (boxes->box[a]->y + boxes->box[a]->h - 1)

// sort by the bottom-left corner of the box
class YSorter {  // concept: stl/StrictWeakOrdering
 public:
  YSorter(const BOXA *iboxes)
      : boxes(iboxes) {}

  bool operator() (int x, int y) {
    return BY(x) < BY(y);
  }

 private:
  const BOXA *const boxes;
};

// sort by the bottom-left corner of the box
class XSorter {  // concept: stl/StrictWeakOrdering
 public:
  XSorter(const BOXA *iboxes)
      : boxes(iboxes) {}

  bool operator() (int x, int y) {
    return boxes->box[x]->x < boxes->box[y]->x;
  }

 private:
  const BOXA *const boxes;
};

// see comment in .h file
void
jbig2enc_textregion(struct jbig2enc_ctx *restrict ctx,
                    /*const*/ std::map<int, int> &symmap,
                    const BOXA *const boxes, const PIXAA *const symbols,
                    NUMA *assignments, int stripwidth, int symbits) {
  // these are the only valid values for stripwidth
  if (stripwidth != 1 && stripwidth != 2 && stripwidth != 4 && stripwidth != 8) abort();

  // sort each box by distance from the top of the page
  const int n = boxes->n;
  // this is a list of indexes into the boxes array
  // indexes into this list of indexes are labled II
  // indexes into the box array are labled I
  std::vector<int> syms(n);
  iota(syms.begin(), syms.end(), 0);
  // sort into height order
  sort(syms.begin(), syms.end(), YSorter(boxes));

  XSorter sorter(boxes);

  int stript = 0;
  int firsts = 0;
  // this is the initial stript value. I don't see why encoding this as zero,
  // then encoding the first stript value as the real start is any worst than
  // encoding this value correctly and then having a 0 value for the first
  // deltat
  jbig2enc_int(ctx, JBIG2_IADT, 0);

  // for each symbol we group it into a strip, which is stripwidth px high
  // for each strip we sort into left-right order
  std::vector<int> strip; // elements of strip: I
  for (int i = 0; i < n;) {   // i: II
    const int height = (BY(syms[i]) / stripwidth) * stripwidth;
    int j;
    strip.clear();
    strip.push_back(syms[i]);

    // now walk until we hit the first symbol which isn't in this strip
    for (j = i + 1; j < n; ++j) {  // j: II
      if (BY(syms[j]) < height) abort();
      if (BY(syms[j]) >= height + stripwidth) {
        // outside strip
        break;
      }
      strip.push_back(syms[j]);
    }

    // now sort the strip into left-right order
    sort(strip.begin(), strip.end(), sorter);
    const int deltat = height - stript;
    //fprintf(stderr, "deltat is %d\n", deltat);
    jbig2enc_int(ctx, JBIG2_IADT, deltat / stripwidth);
    stript = height;
    //fprintf(stderr, "t now: %d\n", stript);

    bool firstsymbol = true;
    int curs = 0;
    // k: iterator(I)
    for (std::vector<int>::const_iterator k = strip.begin(); k != strip.end(); ++k) {
      const int sym = *k;  // sym: I
      //fprintf(stderr, "s: %d\n", boxes->box[sym]->x);
      if (firstsymbol) {
        firstsymbol = false;
        const int deltafs = boxes->box[sym]->x - firsts;
        jbig2enc_int(ctx, JBIG2_IAFS, deltafs);
        firsts += deltafs;
        curs = firsts;
      } else {
        const int deltas = boxes->box[sym]->x - curs;
        jbig2enc_int(ctx, JBIG2_IADS, deltas);
        curs += deltas;
      }

      // if stripwidth is 1, all the t values must be the same so they aren't
      // even encoded
      if (stripwidth > 1) {
        const int deltat = BY(sym) - stript;
        jbig2enc_int(ctx, JBIG2_IAIT, deltat);
      }
      // the symmap maps the number of the symbol from the classifier to the
      // order in while it was written in the symbol dict
      const int symid = symmap[(int)assignments->array[sym]];
      //fprintf(stderr, "sym: %d\n", symid);
      jbig2enc_iaid(ctx, symbits, symid);
      // update curs given the width of the bitmap
      curs += (S((int)assignments->array[sym])->w - 1);
    }
    // terminate the strip
    jbig2enc_oob(ctx, JBIG2_IADS);
    i = j;
  }

  jbig2enc_final(ctx);
}
