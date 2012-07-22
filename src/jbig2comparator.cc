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

#include <map>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#include <leptonica/allheaders.h>

#include <math.h>
#if defined(sun)
#include <sys/types.h>
#else
#include <stdint.h>
#endif

#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t


/**
 * Allocates matrix of preferred size
 * xSize, ySize ... dimensions of the matrix
 */
/*static l_uint32 **
allocate_matrix(int xSize, int ySize) {
  l_uint32 **matrix = new l_uint32*[xSize];
  for (int i = 0; i < xSize; i++) {
    matrix[i] = new l_uint32[ySize];
  }
  return matrix;
}
*/
/**
 * Frees memory used by the matrix
 *
 * **matrix ... matrix which shall be freed from memory
 * xSize ...... number of rows in matrix
 */
/*static void
free_matrix(l_uint32 **matrix, int xSize) {
  for (int i = 0; i < xSize; i++) {
    delete[] matrix[i];
  }
  delete[] matrix;
}


static char *
ints_to_chars(PIX *pix) {
  l_uint32 h = pixGetHeight(pix);
  l_uint32 cpl = (pixGetWidth(pix)+7) / 8;
  fprintf(stderr, "cpl = %d\n", cpl);
  l_uint32 wpl = pixGetWpl(pix);
  l_uint32 *dataAsInts = pixGetData(pix);
  char * dataAsChars;
  dataAsChars = (char*)calloc((h*cpl)+1,sizeof(char));
  int position = 0;
  for (l_uint16 i=0; i < h; i++) {
    for (l_uint16 j=0; j<wpl; j++) {
      l_uint32 num = dataAsInts[i*wpl+j];
      dataAsChars[position++] = (char)((num >> 24) & 0xFF);
      dataAsChars[position++] = (char)((num >> 16) & 0xFF);
      dataAsChars[position++] = (char)((num >> 8) & 0xFF);
      dataAsChars[position++] = (char)(num & 0xFF);
    }
    position -= ((wpl*4) - cpl);
  }
  dataAsChars[position] = '\0';
  return dataAsChars;
}
*/

/**
 * Prints pix bitmap to stderr
 */
void
print_pix(PIX *pix) {
  if (pix == NULL) {
    fprintf(stderr, "Unable to write PIX");
  }
  l_uint32 w = pixGetWidth(pix);
  l_uint32 h = pixGetHeight(pix);
  l_uint32 initUnsigned = 0;
  l_uint32 *pval = &initUnsigned;

  fprintf(stderr, "output before conversion (default) as *l_uint32\n");
  for (l_uint16 i = 0; i < h; i++) {
    for (l_uint16 j = 0; j < w; j++) {
      if (pixGetPixel(pix, j, i, pval)) {
        fprintf(stderr, "unable to read pixel from pix\n");
        break;
      }
      fprintf(stderr, "%d",*pval);
    }
    fprintf(stderr, "\n");
  }

}


/**
 * Compares two pix and tell if they are equivalent by trying to decide if these symbols are equivalent from visual
 * point of view
 *
 * It works by looking for accumulations of differences between two templates
 *
 * If the difference is bigger than concrete percentage of one of templates they are considered different, if such
 * difference doesn't exist than they are equivalent
 *
 * Parts of this function should be recreated using leptonica functions, which should speed up the process, but the
 * principle should remain the same and the result as well
 */
int
are_equivalent(PIX *const firstTemplate, PIX *const secondTemplate) {
  l_int32 w, h, d;

  // checking if they have the same size and depth
  if (!pixSizesEqual(firstTemplate, secondTemplate)) {
      return 0;
  }

  l_int32 firstWpl = pixGetWpl(firstTemplate);
  l_int32 secondWpl = pixGetWpl(secondTemplate);

  if (firstWpl != secondWpl) {
    return 0;
  }

  PIX * pixd;
  pixd = pixXor(NULL, firstTemplate, secondTemplate);

  pixGetDimensions(pixd, &w, &h, &d);

  l_int32 init = 0;
  l_int32 *pcount = &init;
  l_int32 *above = &init;


  // counting number of ON pixels in firstTemplate
  if (pixCountPixels(firstTemplate, pcount, NULL)) {
    fprintf(stderr, "Unable to count pixels\n");
    pixDestroy(&pixd);
    return 0;
  }

  // just for speed up if two symbols are very different
  l_int32 thresh = (*pcount) * 0.25;
  if (pixThresholdPixelSum(pixd, thresh, above, NULL)) {
    fprintf(stderr, "Unable to count pixels of XORed pixes\n");
    pixDestroy(&pixd);
    return 0;
  }


  if ((*above) == 1) {
    pixDestroy(&pixd);
    return 0;
  }

  l_uint32 initUnsigned = 0;
  l_uint32 *pval = &initUnsigned;
  const int divider = 9;
  const int vertical = divider * 2;
  const int horizontal = divider * 2;

  l_uint32 parsedPixCounts[divider][divider];
  l_uint32 horizontalParsedPixCounts[horizontal][divider];
  l_uint32 verticalParsedPixCounts[divider][vertical];

  if (d != 1) {
    return 0;
  }

  int verticalPart = h/divider;
  int horizontalPart = w/divider;

  int horizontalModuleCounter = 0;
  int verticalModuleCounter = 0;


  // counting area of elipse and taking percentage of it as pointThresh
  int a;
  int b;
  if (verticalPart < horizontalPart) {
    a = horizontalPart / 2;
    b = verticalPart / 2;
  } else {
    a = verticalPart / 2;
    b = horizontalPart / 2;
  }

  float pointThresh = a * b * M_PI;
  l_int32 vlineThresh = (verticalPart * (horizontalPart/2))*0.9;
  l_int32 hlineThresh = (horizontalPart * (verticalPart/2))*0.9;


  /*
   * going through submatrixes
   */
  for (int horizontalPosition=0; horizontalPosition < divider; horizontalPosition++) {
    int horizontalStart = horizontalPart*horizontalPosition + horizontalModuleCounter;
    int horizontalEnd;
    if (horizontalPosition == (divider-1)) {
      horizontalModuleCounter = 0;
      horizontalEnd = w;
    } else {
      if (((w - horizontalModuleCounter) % divider)>0) {
        horizontalEnd = horizontalStart + horizontalPart + 1;
        horizontalModuleCounter++;
      } else {
        horizontalEnd = horizontalStart + horizontalPart;
      }
    }

    // zkus spustit ve vlaknech
    for (int verticalPosition=0; verticalPosition < divider; verticalPosition++) {
      int verticalStart = verticalPart*verticalPosition + verticalModuleCounter;
      int verticalEnd;
      if (verticalPosition == (divider-1)) {
        verticalModuleCounter = 0;
        verticalEnd = h;
      } else {
        if (((h - verticalModuleCounter) % divider)>0) {
          verticalEnd = verticalStart + verticalPart + 1;
          verticalModuleCounter++;
        } else {
          verticalEnd = verticalStart + verticalPart;
        }
      }

      // making sum of ON pixels in submatrix and saving the result to matrix of sums
      int leftCounter = 0;
      int rightCounter = 0;
      int downCounter = 0;
      int upCounter = 0;

      int midleOfHorizontalPart = (horizontalStart + horizontalEnd) / 2;
      int midleOfVerticalPart = (verticalStart + verticalEnd) / 2;

      for (int i = horizontalStart; i < horizontalEnd; i++) {
        for (int j = verticalStart; j < verticalEnd; j++) {
          if (pixGetPixel(pixd, i, j, pval)) {
            fprintf(stderr, "unable to read pixel from pix\n");
            break;
          }

          if (*pval == 1) {
            if (i < midleOfHorizontalPart) {
              leftCounter++;
            } else {
              rightCounter++;
            }
            if (j < midleOfVerticalPart) {
              upCounter++;
            } else {
              downCounter++;
            }
          }
        }
      }
      parsedPixCounts[horizontalPosition][verticalPosition] = leftCounter + rightCounter;

      horizontalParsedPixCounts[horizontalPosition*2][verticalPosition] = leftCounter;
      horizontalParsedPixCounts[(horizontalPosition*2)+1][verticalPosition] = rightCounter;

      verticalParsedPixCounts[horizontalPosition][verticalPosition*2] = upCounter;
      verticalParsedPixCounts[horizontalPosition][(verticalPosition*2)+1] = downCounter;

    }
  }

  // destroying XORed pix -- all needed informations are gathered already
  pixDestroy(&pixd);

  // checking for horizontal lines
    for (int i = 0; (i < (divider*2)-1); i++) {
    for (int j = 0; j < (divider-1); j++) {
      int horizontalSum = 0;
      for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
          horizontalSum += horizontalParsedPixCounts[i+x][j+y];
        }
      }
      if (horizontalSum > hlineThresh) {
        return 0;
      }
    }
  }

  // checking for vertical lines
  for (int i = 0; i < (divider-1); i++) {
    for (int j = 0; j < ((divider*2)-1); j++) {
      int verticalSum = 0;
      for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
        verticalSum += verticalParsedPixCounts[i+x][j+y];
        }
      }
      if (verticalSum > vlineThresh) {
        return 0;
      }
    }
  }

  // checking for (cross lines)
  for (int i = 0; i < (divider - 2); i++) {
    for (int j = 0; j < (divider - 2); j++) {
      int leftCross = 0;
      int rightCross = 0;
      for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
          if (x == y) {
            leftCross += parsedPixCounts[i+x][j+y];
          }
          if ((2-x) == y) {
            rightCross += parsedPixCounts[i+x][j+y];
          }
        }
      }
      if ((leftCross > hlineThresh) || (rightCross > hlineThresh)) {
        return 0;
      }
    }
  }

  /*
   * checking if four submatrixes of xored PIX data contains more ON pixels
   * than concrete percentage of ON pixels of firstTemplate
   */
  for (int i = 0; i < (divider-1); i++) {
    for (int j = 0; j < (divider-1); j++) {
      int sum = 0;
      for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
          sum += parsedPixCounts[i+x][j+y];
        }
      }
      if (sum > pointThresh) {
        return 0;
      }
    }
  }
  return 1;
}
