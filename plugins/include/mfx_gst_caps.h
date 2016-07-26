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

#ifndef __MFX_GST_CAPS_H__
#define __MFX_GST_CAPS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define MFX_GST_CAPS_NV12 \
  "video/x-raw, " \
  "format = (string){ NV12 }, " \
  "width = (int)[ 1, 4096 ], " \
  "height = (int)[ 1, 4096 ], " \
  "framerate = (fraction)[ 1/1, 100/1 ], " \
  "pixel-aspect-ratio = (fraction)[ 0/1, 100/1]"

#define MFX_GST_CAPS_I420 \
  "video/x-raw, " \
  "format = (string){ I420 }, " \
  "width = (int)[ 1, 4096 ], " \
  "height = (int)[ 1, 4096 ], " \
  "framerate = (fraction)[ 1/1, 100/1 ], " \
  "pixel-aspect-ratio = (fraction)[ 0/1, 100/1]"

#define MFX_GST_CAPS_YV12 \
  "video/x-raw, " \
  "format = (string){ YV12 }, " \
  "width = (int)[ 1, 4096 ], " \
  "height = (int)[ 1, 4096 ], " \
  "framerate = (fraction)[ 1/1, 100/1 ], " \
  "pixel-aspect-ratio = (fraction)[ 0/1, 100/1]"

#define MFX_GST_CAPS_YUY2 \
  "video/x-raw, " \
  "format = (string){ YUY2 }, " \
  "width = (int)[ 1, 4096 ], " \
  "height = (int)[ 1, 4096 ], " \
  "framerate = (fraction)[ 1/1, 100/1 ], " \
  "pixel-aspect-ratio = (fraction)[ 0/1, 100/1]"

#define MFX_GST_CAPS_UYVY \
  "video/x-raw, " \
  "format = (string){ UYVY }, " \
  "width = (int)[ 1, 4096 ], " \
  "height = (int)[ 1, 4096 ], " \
  "framerate = (fraction)[ 1/1, 100/1 ], " \
  "pixel-aspect-ratio = (fraction)[ 0/1, 100/1]"

#define MFX_GST_CAPS_BGRA \
  "video/x-raw, " \
  "format = (string){ BGRA }, " \
  "width = (int)[ 1, 4096 ], " \
  "height = (int)[ 1, 4096 ], " \
  "framerate = (fraction)[ 1/1, 100/1 ], " \
  "pixel-aspect-ratio = (fraction)[ 0/1, 100/1]"

#define MFX_GST_CAPS_BGRX \
  "video/x-raw, " \
  "format = (string){ BGRx }, " \
  "width = (int)[ 1, 4096 ], " \
  "height = (int)[ 1, 4096 ], " \
  "framerate = (fraction)[ 1/1, 100/1 ], " \
  "pixel-aspect-ratio = (fraction)[ 0/1, 100/1]"

#define MFX_GST_CAPS_H264 \
  "video/x-h264, " \
  "stream-format = (string) { byte-stream, avc }, " \
  "alignment = (string) au, " \
  "width = (int)[ 0, 4096 ], " \
  "height=(int)[ 0, 4096 ], " \
  "framerate = (fraction)[ 0/1, 100/1 ], " \
  "pixel-aspect-ratio= (fraction) [ 0/1, 100/1 ]"

#define MFX_GST_CAPS_H265 \
  "video/x-h265, " \
  "stream-format = (string) { byte-stream, hvc1 }, " \
  "alignment = (string) au, " \
  "width = (int)[ 0, 4096 ], " \
  "height=(int)[ 0, 4096 ], " \
  "framerate = (fraction)[ 0/1, 100/1 ], " \
  "pixel-aspect-ratio= (fraction) [ 0/1, 100/1 ]"

#define MFX_GST_CAPS_JPEG \
  "image/jpeg, " \
  "width = (int)[ 0, 4096 ], " \
  "height=(int)[ 0, 4096 ], " \
  "framerate = (fraction)[ 0/1, 100/1 ] "

#define MFX_GST_CAPS_MFX_FRAME_SURFACE \
  "video/x-raw(memory:mfxFrameSurface), " \
  "width = (int)[ 0, 4096 ], " \
  "height=(int)[ 0, 4096 ], " \
  "framerate = (fraction)[ 0/1, 100/1 ], " \
  "pixel-aspect-ratio= (fraction) [ 0/1, 100/1 ]"

#define MFX_GST_CAPS_VPP_INTERLACE_MODES \
  "interlace-mode = (string){ progressive, interleaved, mixed }"

G_END_DECLS

#endif
