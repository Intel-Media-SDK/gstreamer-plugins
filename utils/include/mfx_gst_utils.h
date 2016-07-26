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

#ifndef __MFX_GST_UTILS_H__
#define __MFX_GST_UTILS_H__

#include <string.h>
#include "mfx_defs.h"
#include "mfx_gst_props.h"

struct MfxGstTypeDefinition
{
  // input parameters:
  GType parent;
  const gchar *type_name;
  guint class_size;
  GClassInitFunc class_init;
  guint instance_size;
  GInstanceInitFunc instance_init;
  GTypeFlags flags;
  // output parameters:
  volatile gsize type_id;
};

extern GType mfx_gst_get_type(MfxGstTypeDefinition& def);

extern bool is_dmabuf_mode(GstPad * pad);

extern bool is_alternate_mode(GstPad * pad);

extern bool mfx_gst_caps_contains_feature(GstCaps * caps, const char * feature);

extern bool mfx_gst_peer_supports_feature(GstCaps * caps, const char * feature);

extern const mfxU32 get_mfx_picstruct(GstBuffer* buffer, GstVideoInterlaceMode mode = GST_VIDEO_INTERLACE_MODE_PROGRESSIVE);

extern const mfxU32 get_gst_picstruct(mfxU32 pic_struct);

inline const char * mfx_gst_interlace_mode_to_string(GstVideoInterlaceMode mode)
{
  MFX_DEBUG_TRACE_FUNC;

  switch(mode) {
    case GST_VIDEO_INTERLACE_MODE_PROGRESSIVE:
      return "progressive";
    case GST_VIDEO_INTERLACE_MODE_INTERLEAVED:
      return "interleaved";
    case GST_VIDEO_INTERLACE_MODE_MIXED:
      return "mixed";
    case GST_VIDEO_INTERLACE_MODE_FIELDS:
      return "fields";
    default:
      g_assert_not_reached();
      return NULL;
  }
}


#endif // __MFX_GST_UTILS_H__
