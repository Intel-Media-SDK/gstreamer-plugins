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

#ifndef __MFX_GST_PROPS_H__
#define __MFX_GST_PROPS_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include "mfx_gst_debug.h"
#include "mfxstructures.h"

enum {
  /* Common properties */
  PROP_MemoryType = 1, // Output memory type
  PROP_AsyncDepth,
  PROP_Implementation,
  /* Plugin properties for encoder */
  PROP_GopPicSize = 1001,
  PROP_GopRefDist,
  PROP_RateControlMethod,
  PROP_QPI,
  PROP_QPP,
  PROP_QPB,
  PROP_TargetKbps,
  PROP_MaxKbps,
  PROP_InitialDelayInKB,
  PROP_BRCParamMultiplier,
  PROP_TargetUsage,
  PROP_NumSlice,
  PROP_JpegQuality,
  /* Plugin properties for vpp */
  PROP_vpp_out_width = 2001,
  PROP_vpp_out_height,
  PROP_vpp_out_format,
  PROP_vpp_deinterlacing,
  PROP_vpp_passthrough,
  /* Plugin properties for decoder */
  PROP_dec_first = 3001,
};

enum FeatureOption {
  FEATURE_OFF = 0x00,
  FEATURE_ON  = 0x01,
};

/* PROP_MemoryType */
enum MemType {
  SYSTEM_MEMORY = 0x00,
  VIDEO_MEMORY  = 0x01,
};

/* PROP_StreamFormat */
enum StreamFormat {
  MFX_STREAM_FORMAT_UNKNOWN = 0,
  MFX_STREAM_FORMAT_ANNEX_B,
  MFX_STREAM_FORMAT_AVCC,
  MFX_STREAM_FORMAT_HVC1,
};

struct MfxGstPluginProperty
{
  // property usage flags
  enum {
    uDefault = 0,
    // property holds initial component parameters (in default value)
    uInit = 0x1
  };

  guint id;
  guint usage;
  guint type;
  const char* name;
  const char* nick;
  const char* descr;
  union {
    struct {
      gint min;
      gint max;
      gint def;
    } vInt;
    struct {
      GType (*get_type)();
      gint def;
    } vEnum;
  };
  GParamFlags flags;

  // this C++ struct should define static member functions only
  static GParamSpec* get_param_spec(MfxGstPluginProperty* prop);
  static GParamSpec* get_param_spec_int(MfxGstPluginProperty* prop);
  static GParamSpec* get_param_spec_enum(MfxGstPluginProperty* prop);
};

G_BEGIN_DECLS

extern GType get_type__mfx_ratecontrol(void);
extern GType get_type__target_usage(void);
extern GType get_type__memory_type(void);
extern GType get_type__stream_format(void);
extern GType get_type__feature_option(void);
inline GType get_type__color_format(void)
{
  return GST_TYPE_VIDEO_FORMAT;
}
extern GType get_type__deinterlacing(void);
extern GType get_type__implementation_type(void);

G_END_DECLS

#endif
