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
#include "mfx_gst_plugin_vpp.h"

G_BEGIN_DECLS

const char __defs__plugin_name[] = "mfx_gst_vpp";
const char __defs__plugin_desc[] = "It contains vpp plugin.";
const char __defs__caps_vpp_raw_in[] =
  MFX_GST_CAPS_MFX_FRAME_SURFACE ", " MFX_GST_CAPS_VPP_INTERLACE_MODES "; "
  MFX_GST_CAPS_NV12 ", " MFX_GST_CAPS_VPP_INTERLACE_MODES "; "
  MFX_GST_CAPS_YV12 ", " MFX_GST_CAPS_VPP_INTERLACE_MODES "; "
  MFX_GST_CAPS_YUY2 ", " MFX_GST_CAPS_VPP_INTERLACE_MODES "; "
  MFX_GST_CAPS_UYVY ", " MFX_GST_CAPS_VPP_INTERLACE_MODES "; "
  MFX_GST_CAPS_I420 ", " MFX_GST_CAPS_VPP_INTERLACE_MODES "; "
  MFX_GST_CAPS_BGRA "; "
  MFX_GST_CAPS_BGRX;

const char __defs__caps_vpp_raw_out[] =
  MFX_GST_CAPS_MFX_FRAME_SURFACE "; "
  MFX_GST_CAPS_NV12;

static mfx_GstPadFunctions __defs__plugin_sink_pad_funcs =
{
  mfx_gst_vpp_query,
  mfx_gst_plugin_vpp_input_event,
  mfx_gst_vpp_chain
};

static mfx_GstPadFunctions __defs__plugin_src_pad_funcs =
{
  mfx_gst_vpp_query,
  mfx_gst_plugin_vpp_output_event,
  NULL
};

mfx_GstPadTemplate __defs__plugin_pad_templ[] =
{
  {
    GST_STATIC_PAD_TEMPLATE(
      "sink",
      GST_PAD_SINK,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS(__defs__caps_vpp_raw_in)),
    &__defs__plugin_sink_pad_funcs
  },
  {
    GST_STATIC_PAD_TEMPLATE(
      "src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS(__defs__caps_vpp_raw_out)),
    &__defs__plugin_src_pad_funcs
  }
};

MfxGstPluginProperty __defs_plugin_properties[] =
{
  {
    PROP_MemoryType,
    MfxGstPluginProperty::uInit,
    G_TYPE_ENUM,
    "memory", "memory", "Output memory type",
    {
      .vEnum = {
        get_type__memory_type,
        VIDEO_MEMORY
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_vpp_out_width,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "width", "width", "Output frames width",
    {
      .vInt = {
        0, 4096, 0
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_vpp_out_height,
    MfxGstPluginProperty::uInit,
    G_TYPE_INT,
    "height", "height", "Output frames height",
    {
      .vInt = {
        0, 4096, 0
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_vpp_out_format,
    MfxGstPluginProperty::uInit,
    G_TYPE_ENUM,
    "format", "format", "Output color format",
    {
      .vEnum = {
        get_type__color_format,
        GST_VIDEO_FORMAT_NV12
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_vpp_deinterlacing,
    MfxGstPluginProperty::uInit,
    G_TYPE_ENUM,
    "di", "di", "Deinterlacing type",
    {
      .vEnum = {
        get_type__deinterlacing,
        FEATURE_OFF
      }
    },
    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  },
  {
    PROP_vpp_passthrough,
    MfxGstPluginProperty::uInit,
    G_TYPE_ENUM,
    "passthrough", "passthrough", "Enable passthrough mode",
    {
      .vEnum = {
        get_type__feature_option,
        FEATURE_OFF
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
  {
    PROP_Implementation,
    MfxGstPluginProperty::uInit,
    G_TYPE_ENUM,
    "implementation", "implementation", "Implementation type ",
    {
      .vEnum = {
        get_type__implementation_type,
        MFX_IMPL_HARDWARE
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
  .longname = "Intel(r) Media SDK Video Processing GStreamer Plug-In",
  .classification = "Converter/Video",
  .description = "Transforms input video flow into output one",
  .author = MFX_GST_AUTHOR,
  .pad_templ = __defs__plugin_pad_templ,
  .pad_templ_n = MSDK_ARRAY_SIZE(__defs__plugin_pad_templ),
  .properties = __defs_plugin_properties,
  .properties_n = MSDK_ARRAY_SIZE(__defs_plugin_properties),
  .type_find = NULL,
  .type_ext = "",
  .type_name = "",
  .type_caps = "",
  .data_init = mfx_gst_plugin_vpp_data_init,
  .data_dispose = mfx_gst_plugin_vpp_data_dispose,
  .change_state = mfx_gst_plugin_vpp_change_state,
  .get_property = mfx_gst_vpp_get_property,
  .set_property = mfx_gst_vpp_set_property,
};

G_END_DECLS
