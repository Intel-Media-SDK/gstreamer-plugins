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

#ifndef __MFX_GST_PLUGIN_H__
#define __MFX_GST_PLUGIN_H__

#include <stdlib.h>
#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include "mfx_defs.h"
#include "mfx_gst_debug.h"
#include "mfx_mfx_debug.h"
#include "mfx_vaapi_debug.h"
#include "mfx_gst_package_defs.h"
#include "mfx_gst_props.h"

struct mfxGstCapsData
{
  mfxGstCapsData() :
    codecid(0)
  , width(0)
  , height(0)
  , crop_x(0)
  , crop_y(0)
  , crop_w(0)
  , crop_h(0)
  , framerate_n(0)
  , framerate_d(0)
  , in_memory(SYSTEM_MEMORY)
  , out_memory(SYSTEM_MEMORY)
  {
  }

  union {
    mfxU32 codecid;
    mfxU32 fourcc;
  };
  gint width;
  gint height;
  gint crop_x;
  gint crop_y;
  gint crop_w;
  gint crop_h;
  gint framerate_n;
  gint framerate_d;
  MemType in_memory;
  MemType out_memory;
};

G_BEGIN_DECLS

struct _mfx_GstPlugin
{
  GstElement element;
  GList* pads; /* list of the pads of mfx_GstPad type */
  const MfxGstPluginProperty* props; /* set of supported plugin properties */
  size_t props_n; /* number of properties */
  gpointer data;
  gboolean codec_inited;
};
typedef struct _mfx_GstPlugin mfx_GstPlugin;

#define MFX_GST_GET_PAD(L) (mfx_GstPad*)((L)->data)

struct _mfx_GstBaseClass
{
  GstElementClass element;
  // TODO delete pad_templ: looks like we don't need this...
//  GstPadTemplate* pad_templ[__defs__plugin_pad_templ_n];
};
typedef struct _mfx_GstBaseClass mfx_GstBaseClass;

typedef struct _mfx_GstPad mfx_GstPad;

struct _mfx_GstPadFunctions
{
  gboolean (*query)(mfx_GstPad *pad, mfx_GstPlugin *plugin, GstQuery *query);
  gboolean (*event)(mfx_GstPad *pad, mfx_GstPlugin *plugin, GstEvent *data);
  GstFlowReturn (*chain)(mfx_GstPad *pad, mfx_GstPlugin *plugin, GstBuffer *data);
};
typedef struct _mfx_GstPadFunctions mfx_GstPadFunctions;

struct _mfx_GstPadTemplate
{
  GstStaticPadTemplate gst_templ; /* GStreamer template of the pad */
  mfx_GstPadFunctions* funcs; /* list of functions to be associated with the pad */
};
typedef struct _mfx_GstPadTemplate mfx_GstPadTemplate;

struct _mfx_GstPad
{
  GstPad* pad;
  mfx_GstPadTemplate* templ;
};

extern mfx_GstPad* mfx_gst_plugin_get_pad_by_direction(
  mfx_GstPlugin* plugin,
  GstPadDirection direction);

extern GstPad* mfx_gst_plugin_get_src_pad(
  mfx_GstPlugin* plugin);

extern GstPad* mfx_gst_plugin_get_sink_pad(
  mfx_GstPlugin* plugin);

/* Plugin-specific declarations */
extern const char __defs__plugin_name[];
extern const char __defs__plugin_desc[];

struct MfxGstPluginDef
{
  guint rank;

  const char* longname;
  const char* classification;
  const char* description;
  const char* author;

  mfx_GstPadTemplate* pad_templ;
  size_t pad_templ_n;

  MfxGstPluginProperty* properties;
  size_t properties_n;

  GstTypeFindFunction type_find;
  const char* type_ext;
  const char* type_name;
  const char* type_caps;

  gpointer (*data_init)(mfx_GstPlugin* plugin);
  gpointer (*data_dispose)(mfx_GstPlugin* plugin);
  GstStateChangeReturn (*change_state)(mfx_GstPlugin *element, GstStateChange state);
  bool (*get_property)(mfx_GstPlugin* plugin, guint id, GValue *val, GParamSpec *ps);
  bool (*set_property)(mfx_GstPlugin* plugin, guint id, const GValue *val, GParamSpec *ps);
};

extern MfxGstPluginDef __plugin_def;

G_END_DECLS

#endif
