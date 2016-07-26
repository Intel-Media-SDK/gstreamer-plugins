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

#include "mfx_gst_plugin_venc.h"
#include "mfx_gst_executor_venc.h"
#include "mfx_utils.h"

#include <algorithm>

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstplugin_venc"

G_BEGIN_DECLS

void mfx_gst_plugin_venc_notify_error(mfx_GstPlugin* plugin)
{
  GST_ELEMENT_ERROR(&plugin->element, RESOURCE, NOT_FOUND, (("No file name specified for reading.")), (NULL));
//    NULL/*("No file name specified for reading.")*/, (NULL));
//gst_element_message_full(&plugin->element, GST_MESSAGE_ERROR, gst_message_type_to_quark(GST_MESSAGE_ERROR), GST_MESSAGE_ERROR, NULL, NULL, __FILE__, __FUNCTION__, __LINE__);
}

void mfx_gst_plugin_venc_buffer_ready(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstBuffer *buffer)
{
  MFX_DEBUG_TRACE_FUNC;
  GstFlowReturn gf;

  gf = gst_pad_push(mfxpad->pad, buffer);
  MFX_DEBUG_TRACE__GstFlowReturn(gf);

  if (gf != GST_FLOW_OK) {
    mfx_gst_plugin_venc_notify_error(plugin);
  }
}

gpointer mfx_gst_plugin_venc_data_init(mfx_GstPlugin* plugin)
{
  MFX_DEBUG_TRACE_FUNC;
  mfxGstPluginVencData* data;
  mfx_GstPad* mfxpad = mfx_gst_plugin_get_pad_by_direction(plugin, GST_PAD_SRC);

  if (!mfxpad) {
    MFX_DEBUG_TRACE_MSG("bug: no source pad!");
    return NULL;
  }
  try {
    data = new mfxGstPluginVencData(plugin);

    data->SetBufferReadyCallback(
      std::bind(
        &mfx_gst_plugin_venc_buffer_ready,
        mfxpad,
        plugin,
        std::placeholders::_1));

    data->SetNotifyErrorCallback(
      std::bind(
        &mfx_gst_plugin_venc_notify_error,
        plugin));
  }
  catch(...) {
    data = NULL;
  }
  MFX_DEBUG_TRACE_P(data);
  return data;
}

gpointer mfx_gst_plugin_venc_data_dispose(mfx_GstPlugin* plugin)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_P(plugin);
  if (plugin && plugin->data) {
    delete (mfxGstPluginVencData*)plugin->data;
    plugin->data = NULL;
  }
  return NULL;
}

GstStateChangeReturn mfx_gst_plugin_venc_change_state(
  mfx_GstPlugin *plugin,
  GstStateChange state)
{
  MFX_DEBUG_TRACE_FUNC;
  GstElementClass *element_class =
    (GstElementClass *)g_type_class_ref(GST_TYPE_ELEMENT);
  GstStateChangeReturn gs_res;

  MFX_DEBUG_TRACE_P(plugin);
  MFX_DEBUG_TRACE__GstStateChange(state);

  if (!plugin) {
    return GST_STATE_CHANGE_FAILURE;
  }

  mfxGstPluginVencData* venc = (mfxGstPluginVencData*)plugin->data;

  if (GST_STATE_CHANGE_NULL_TO_READY == state) {
    if (!venc->InitBase()) {
      // This failure means that something critical has happenned. For example,
      // mediasdk library or some plugin was not found or GPU failed to initialize.
      MFX_DEBUG_TRACE_MSG("failed to initialize mediasdk for processing");
      return GST_STATE_CHANGE_FAILURE;
    }
  }
  gs_res = element_class->change_state(&(plugin->element), state);
  if (GST_STATE_CHANGE_READY_TO_NULL == state) {
    venc->Dispose();
  }
  MFX_DEBUG_TRACE__GstStateChangeReturn(gs_res);
  return gs_res;
}

gboolean mfx_gst_venc_query(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstQuery *query)
{
  MFX_DEBUG_TRACE_FUNC;
  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);

  mfxGstPluginVencData* venc = (mfxGstPluginVencData*)plugin->data;
  if (GST_QUERY_TYPE(query) == GST_QUERY_CONTEXT) {
    return venc->RunQueryContext(query);
  }
  return gst_pad_query_default(mfxpad->pad, (GstObject*)plugin, query);
}

gboolean mfx_gst_plugin_venc_input_event(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstEvent *event)
{
  MFX_DEBUG_TRACE_FUNC;
  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);
  MFX_DEBUG_TRACE__GstEventType(event->type);

  mfxGstPluginVencData* venc = (mfxGstPluginVencData*)plugin->data;

  if (GST_EVENT_CAPS == event->type) {
    MFX_DEBUG_TRACE("GST_EVENT_CAPS");
    gint width, height, framerate_n, framerate_d, crop_x = 0, crop_y = 0, crop_w = 0, crop_h = 0;
    MemType memory = SYSTEM_MEMORY;
    const gchar* interlace_mode;
    const gchar* mime;
    const GstStructure *gststruct;
    GstCaps* caps = NULL;

    gst_event_parse_caps(event, &caps);
    MFX_DEBUG_TRACE_P(caps);
    if (!caps || !gst_caps_is_fixed(caps)) {
      MFX_DEBUG_TRACE_MSG("no caps or they are not fixed");
      return FALSE;
    }
    gststruct = gst_caps_get_structure(caps, 0);
    if (!gststruct) {
      MFX_DEBUG_TRACE_MSG("no caps structure");
      return FALSE;
    }
    {
      gchar* str = NULL;
      MFX_DEBUG_TRACE_S(str = gst_structure_to_string(gststruct));
      if (str) g_free(str);
    }

    if (!gst_structure_get_int(gststruct, "width", &width) ||
        !gst_structure_get_int(gststruct, "height", &height) ||
        !(interlace_mode = gst_structure_get_string(gststruct, "interlace-mode")) ||
        !gst_structure_get_fraction(gststruct, "framerate", &framerate_n, &framerate_d)) {
      MFX_DEBUG_TRACE_MSG("failed to get required field(s) from the caps structure");
      return FALSE;
    }
    if (!gst_structure_get_int(gststruct, "crop-x", &crop_x) ||
        !gst_structure_get_int(gststruct, "crop-y", &crop_y) ||
        !gst_structure_get_int(gststruct, "crop-w", &crop_w) ||
        !gst_structure_get_int(gststruct, "crop-h", &crop_h)) {
      MFX_DEBUG_TRACE_MSG("failed to get crops: using width/height");
      crop_w = width;
      crop_h = height;
    }

    for (mfxU32 i = 0; i < gst_caps_get_size(caps); ++i) {
      GstCapsFeatures* features = gst_caps_get_features(caps, i);
      if (gst_caps_features_is_any(features)) {
        continue;
      }
      if (gst_caps_features_contains(features, GST_CAPS_FEATURE_MFX_FRAME_SURFACE_MEMORY)) {
        memory = VIDEO_MEMORY;
        break;
      }
    }

    mfx_GstPad* srcpad = mfx_gst_plugin_get_pad_by_direction(plugin, GST_PAD_SRC);
    if (!srcpad) {
      MFX_DEBUG_TRACE_MSG("bug: no source pad!");
      return FALSE;
    }
    GstCaps* allowed_caps = gst_pad_get_allowed_caps(srcpad->pad);
    if (!allowed_caps) {
      allowed_caps = gst_caps_copy(gst_pad_get_pad_template_caps(srcpad->pad));
    }
    if (!allowed_caps) {
      MFX_DEBUG_TRACE_MSG("failed to get allowed caps");
      return FALSE;
    }

    mime = gst_structure_get_name(gst_caps_get_structure(allowed_caps, 0));

    MFX_DEBUG_TRACE_I32(width);
    MFX_DEBUG_TRACE_I32(height);
    MFX_DEBUG_TRACE_I32(framerate_n);
    MFX_DEBUG_TRACE_I32(framerate_d);
    MFX_DEBUG_TRACE_S(interlace_mode);

    mfxGstCapsData capsdata;

    capsdata.codecid = mfx_codecid_from_mime(mime);
    capsdata.width = (mfxU16)width;
    capsdata.height = (mfxU16)height;
    capsdata.crop_x = (mfxU16)crop_x;
    capsdata.crop_y = (mfxU16)crop_y;
    capsdata.crop_w = (mfxU16)crop_w;
    capsdata.crop_h = (mfxU16)crop_h;
    capsdata.framerate_n = (mfxU16)framerate_n;
    capsdata.framerate_d = (mfxU16)framerate_d;
    capsdata.in_memory = memory;
    if (strcmp(interlace_mode, "progressive")) {
      MFX_DEBUG_TRACE_MSG("unsupported interlace mode");
      return FALSE;
    }
    if (!venc->CheckAndSetCaps(capsdata)) {
      MFX_DEBUG_TRACE_MSG("failed to initialize encoder");
      return FALSE;
    }
    if (!venc->Init()) { // ?? here ??
      MFX_DEBUG_TRACE_MSG("failed to initialize encoder");
      return FALSE;
    }
    if (!venc->StartTaskThread()) {
      MFX_DEBUG_TRACE_MSG("failed to start encoding task thread");
      return FALSE;
    }
    if (!venc->StartSyncThread()) {
      MFX_DEBUG_TRACE_MSG("failed to start encoding sync thread");
      return FALSE;
    }

    GstCaps* encoder_caps = venc->GetOutCaps(allowed_caps);
    if (!encoder_caps) {
      MFX_DEBUG_TRACE_MSG("failed to generate encoder caps");
      gst_caps_unref(allowed_caps);
      return FALSE;
    }

    GstCaps* out_caps = gst_caps_intersect(allowed_caps, encoder_caps);
    //gst_caps_unref(caps); // TODO do we need this?
    gst_caps_unref(allowed_caps);
    gst_caps_unref(encoder_caps);

    if (!out_caps) {
      MFX_DEBUG_TRACE_MSG("failed to find appropriate caps");
      return FALSE;
    }

    if (!gst_pad_set_caps(srcpad->pad, out_caps)) {
      MFX_DEBUG_TRACE_MSG("failed to set caps on src pad");
      gst_caps_unref(out_caps);
      return FALSE;
    }

    gst_caps_unref(out_caps);
    return TRUE;
  }
  else if (GST_EVENT_EOS == event->type) {
    MFX_DEBUG_TRACE("GST_EVENT_EOS");
    if (venc->PostEndOfStreamTask(
      std::bind(
        gst_pad_event_default,
        mfxpad->pad,
        (GstObject*)plugin,
        event))) {
      return true;
    }
    // forwarding EOS right away in case we failed to post a task
    return gst_pad_event_default(mfxpad->pad, (GstObject*)plugin, event);
  }
  return gst_pad_event_default(mfxpad->pad, (GstObject*)plugin, event);
}

gboolean mfx_gst_plugin_venc_output_event(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstEvent *event)
{
  MFX_DEBUG_TRACE_FUNC;
  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);
  MFX_DEBUG_TRACE__GstEventType(event->type);

  return gst_pad_event_default(mfxpad->pad, (GstObject*)plugin, event);
}

GstFlowReturn mfx_gst_venc_chain(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstBuffer *buffer)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);

  if (!plugin || !buffer) {
    return GST_FLOW_ERROR;
  }

  mfxGstPluginVencData* venc = (mfxGstPluginVencData*)plugin->data;
  return (venc->PostEncodeTask(buffer))? GST_FLOW_OK: GST_FLOW_ERROR;
}

bool mfx_gst_venc_get_property(
  mfx_GstPlugin *plugin,
  guint id, GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxGstPluginVencData* venc = (mfxGstPluginVencData*)plugin->data;
  return venc->GetProperty(id, val, ps);
}

bool mfx_gst_venc_set_property(
  mfx_GstPlugin *plugin,
  guint id, const GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxGstPluginVencData* venc = (mfxGstPluginVencData*)plugin->data;
  return venc->SetProperty(id, val, ps);
}

G_END_DECLS
