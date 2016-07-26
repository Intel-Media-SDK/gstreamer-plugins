/**********************************************************************************

Copyright (C) 2005-2016 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**********************************************************************************/

#ifndef __MFX_DEFS_H__
#define __MFX_DEFS_H__

#include "mfxdefs.h"
#include "mfxcommon.h"
#include "mfxjpeg.h"

#define MSDK_PRIMARY_VIDEOCARD "/dev/dri/card0"

#define MSDK_IS_ALIGNED(X,N) (((X) & ((N) - 1)) == 0)

#define MSDK_ALIGN(X,Y) (((mfxU32)((X)+(Y-1))) & (~ (mfxU32)(Y-1)))
#define MSDK_ALIGN2(X) MSDK_ALIGN(X,2)
#define MSDK_ALIGN4(X) MSDK_ALIGN(X,4)
#define MSDK_ALIGN16(X) MSDK_ALIGN(X,16)
#define MSDK_ALIGN32(X) MSDK_ALIGN(X,32)

#define MSDK_TIMEOUT 0xEFFFFFFF

#define MSDK_FREE(P) \
  do { \
    if (P) { free(P); P = NULL; } \
  } while(0)

#define MSDK_DELETE(P) \
  do { \
    if (P) { delete P; P = NULL; } \
  } while(0)

#define MSDK_OBJECT_UNREF(P) \
  do { \
    if (P) { g_object_unref(P); P = NULL; } \
  } while(0)

#define MSDK_G_FREE(P) \
  do { \
    if (P) { g_free(P); P = NULL; } \
  } while(0)

#define MSDK_ARRAY_SIZE(A) sizeof(A)/sizeof(A[0])
#define MSDK_ZERO_MEM(M) \
  do { \
    memset(&M, 0, sizeof(M)); \
  } while(0)

#define MSDK_MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MSDK_MIN(A, B) (((A) < (B)) ? (A) : (B))

enum {
    MFX_FOURCC_I420 = MFX_MAKEFOURCC('I','4','2','0'),
};

#endif
