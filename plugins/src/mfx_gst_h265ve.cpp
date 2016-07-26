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

#define MFX_DEBUG_FILE_INIT

#include "mfx_gst_caps.h"
#include "mfx_gst_plugin.h"
#include "mfx_gst_plugin_venc.h"

G_BEGIN_DECLS

const char __defs__plugin_name[] = "mfx_gst_h265ve";
const char __defs__plugin_desc[] = "It contains H.265 encoder plugin.";
const char __defs__caps_h265ve_raw[] =
  MFX_GST_CAPS_MFX_FRAME_SURFACE "; "
  MFX_GST_CAPS_NV12;

static mfx_GstPadFunctions __defs__plugin_sink_pad_funcs =
{
  mfx_gst_venc_query,
  mfx_gst_plugin_venc_input_event,
  mfx_gst_venc_chain
};

static mfx_GstPadFunctions __defs__plugin_src_pad_funcs =
{
  mfx_gst_venc_query,
  mfx_gst_plugin_venc_output_event,
  NULL
};

mfx_GstPadTemplate __defs__plugin_pad_templ[] =
{
  {
    GST_STATIC_PAD_TEMPLATE(
      "sink",
      GST_PAD_SINK,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS(__defs__caps_h265ve_raw)),
    &__defs__plugin_sink_pad_funcs
  },
  {
    GST_STATIC_PAD_TEMPLATE(
      "src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS(MFX_GST_CAPS_H265)),
    &__defs__plugin_src_pad_funcs
  }
};

MfxGstPluginProperty __defs_plugin_properties[] =
{
  {
    PROP_GopPicSize,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "GopPicSize", "GopPicSize", "",
    {
      .vInt = {
        0, G_MAXINT, 30
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_GopRefDist,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "GopRefDist", "GopRefDist", "",
    {
      .vInt = {
        0, G_MAXINT, 1
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_RateControlMethod,
    MfxGstPluginProperty::uInit,
    G_TYPE_ENUM,
    "RateControlMethod", "RateControlMethod", "",
    {
      .vEnum = {
        get_type__mfx_ratecontrol,
        MFX_RATECONTROL_CQP
      }
    },
    (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_QPI,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "QPI", "QPI", "",
    {
      .vInt = {
        1, 51, 26
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_QPP,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "QPP", "QPP", "",
    {
      .vInt = {
        1, 51, 26
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_QPB,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "QPB", "QPB", "",
    {
      .vInt = {
        1, 51, 26
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_NumSlice,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "NumSlice", "NumSlice", "",
    {
      .vInt = {
        1, G_MAXINT, 1
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_TargetUsage,
    MfxGstPluginProperty::uInit,
    G_TYPE_ENUM,
    "TargetUsage", "TargetUsage", "",
    {
      .vEnum = {
        get_type__target_usage,
        MFX_TARGETUSAGE_BALANCED
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_AsyncDepth,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "AsyncDepth", "AsyncDepth", "Max number of simultaneous asynchronous operations",
    {
      .vInt = {
        1, 5, 5
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
};

MfxGstPluginDef __plugin_def =
{
  .rank =
#ifdef MFX_GST_PLUGIN_RANK
    MFX_GST_PLUGIN_RANK,
#else
    GST_RANK_PRIMARY + 1,
#endif
  .longname = "Intel(r) Media SDK H.265 Encoder GStreamer Plug-In",
  .classification = "Codec/Encoder/Video",
  .description = "It encodes H.265 video stream.",
  .author = MFX_GST_AUTHOR,
  .pad_templ = __defs__plugin_pad_templ,
  .pad_templ_n = MSDK_ARRAY_SIZE(__defs__plugin_pad_templ),
  .properties = __defs_plugin_properties,
  .properties_n = MSDK_ARRAY_SIZE(__defs_plugin_properties),
  .type_find = NULL,
  .type_ext = "",
  .type_name = "",
  .type_caps = "",
  .data_init = mfx_gst_plugin_venc_data_init,
  .data_dispose = mfx_gst_plugin_venc_data_dispose,
  .change_state = mfx_gst_plugin_venc_change_state,
  .get_property = mfx_gst_venc_get_property,
  .set_property = mfx_gst_venc_set_property,
};

G_END_DECLS
