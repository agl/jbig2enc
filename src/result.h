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

#ifndef _JBIG2_RESULT_H_
#define _JBIG2_RESULT_H_

#include <leptonica/allheaders.h>

/**
 * Structure used as reference (fallback) point, which allows to count distance of two PIXes without using OCR
 */
class Result {
  protected:
    PIX * pix;

  public:
    Result() {}
    Result(PIX *pix) {
      this->pix = pix;
    }

    ~Result() {
      this->pix = NULL;
    }

    PIX * getPix() {
      return pix;
    }

    void setPix(PIX * pix) {
      this->pix = pix;
    }

    virtual float getDistance(Result *result);

};

#endif
