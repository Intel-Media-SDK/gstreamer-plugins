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

#include "mfx_gst_plugin_vdec.h"
#include "mfx_gst_executor_vdec.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstplugin_vdec"

G_BEGIN_DECLS

void mfx_gst_plugin_vdec_notify_error(mfx_GstPlugin* plugin)
{
  GST_ELEMENT_ERROR(&plugin->element, RESOURCE, NOT_FOUND, (("Error")), (NULL));
}

void mfx_gst_plugin_vdec_buffer_ready(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstBuffer *buffer)
{
  MFX_DEBUG_TRACE_FUNC;
  GstFlowReturn gf;

  gf = gst_pad_push(mfxpad->pad, buffer);
  MFX_DEBUG_TRACE__GstFlowReturn(gf);

  if (gf != GST_FLOW_OK) {
    mfx_gst_plugin_vdec_notify_error(plugin);
  }
}

gpointer mfx_gst_plugin_vdec_data_init(mfx_GstPlugin* plugin)
{
  MFX_DEBUG_TRACE_FUNC;
  mfxGstPluginVdecData* data;
  mfx_GstPad* mfxpad = mfx_gst_plugin_get_pad_by_direction(plugin, GST_PAD_SRC);

  if (!mfxpad) {
    MFX_DEBUG_TRACE_MSG("bug: no source pad!");
    return NULL;
  }
  try {
    data = new mfxGstPluginVdecData(plugin);

    data->SetBufferReadyCallback(
      std::bind(
        &mfx_gst_plugin_vdec_buffer_ready,
        mfxpad,
        plugin,
        std::placeholders::_1));

    data->SetNotifyErrorCallback(
      std::bind(
        &mfx_gst_plugin_vdec_notify_error,
        plugin));
  }
  catch(...) {
    data = NULL;
  }
  MFX_DEBUG_TRACE_P(data);
  return data;
}

gpointer mfx_gst_plugin_vdec_data_dispose(mfx_GstPlugin* plugin)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_P(plugin);
  if (plugin && plugin->data) {
    delete (mfxGstPluginVdecData*)plugin->data;
    plugin->data = NULL;
  }
  return NULL;
}

GstStateChangeReturn mfx_gst_plugin_vdec_change_state(
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

  mfxGstPluginVdecData* vdec = (mfxGstPluginVdecData*)plugin->data;

  if (GST_STATE_CHANGE_NULL_TO_READY == state) {
    if (!vdec->InitBase()) {
      MFX_DEBUG_TRACE_MSG("failed to initialize component");
      return GST_STATE_CHANGE_FAILURE;
    }
  }
  gs_res = element_class->change_state(&(plugin->element), state);
  if (GST_STATE_CHANGE_READY_TO_NULL == state) {
    vdec->Dispose();
  }
  MFX_DEBUG_TRACE__GstStateChangeReturn(gs_res);
  return gs_res;
}

gboolean mfx_gst_vdec_query(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstQuery *query)
{
  MFX_DEBUG_TRACE_FUNC;
  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);

  return gst_pad_query_default(mfxpad->pad, (GstObject*)plugin, query);
}

gboolean mfx_gst_plugin_vdec_input_event(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstEvent *event)
{
  MFX_DEBUG_TRACE_FUNC;
  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);
  MFX_DEBUG_TRACE__GstEventType(event->type);

  mfxGstPluginVdecData* vdec = (mfxGstPluginVdecData*)plugin->data;

  if (GST_EVENT_STREAM_START == event->type) {
    if (!vdec->StartTaskThread()) {
      MFX_DEBUG_TRACE_MSG("failed to start encoding task thread");
      return FALSE;
    }
    if (!vdec->StartSyncThread()) {
      MFX_DEBUG_TRACE_MSG("failed to start encoding sync thread");
      return FALSE;
    }
    return gst_pad_event_default(mfxpad->pad, (GstObject*)plugin, event);
  } else if (GST_EVENT_CAPS == event->type) {
    MFX_DEBUG_TRACE_MSG("GST_EVENT_CAPS");

    GstCaps* caps = nullptr;
    gst_event_parse_caps(event, &caps);
    MFX_DEBUG_TRACE_P(caps);

    if (!caps || !gst_caps_is_fixed(caps)) {
      MFX_DEBUG_TRACE_MSG("no caps or they are not fixed");
      return FALSE;
    }

    if (!vdec->CheckAndSetCaps(caps)) {
      MFX_DEBUG_TRACE_MSG("failed to initialize");
      return FALSE;
    }
    auto src_pad = mfx_gst_plugin_get_pad_by_direction(plugin, GST_PAD_SRC);
    if (!src_pad) {
      MFX_DEBUG_TRACE_MSG("bug: no source pad!");
      return FALSE;
    }
    auto src_caps = vdec->CreateOutCaps();
    gboolean ret = gst_pad_set_caps(src_pad->pad, src_caps);
    if (!ret) {
      MFX_DEBUG_TRACE_MSG("failed to set pad caps!");
      return FALSE;
    }
    gst_caps_unref(src_caps);

    return ret;
  } else if (GST_EVENT_EOS == event->type) {
    MFX_DEBUG_TRACE_MSG("GST_EVENT_EOS");
    if (vdec->PostEndOfStreamTask(
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
  else if (GST_EVENT_SEGMENT == event->type) {
    gint64 start = 0;
    GstSegment segment;
    gst_event_copy_segment (event, &segment);

    if (segment.format != GST_FORMAT_TIME) {
      gst_pad_query_convert (mfxpad->pad, segment.format,
                  segment.start, GST_FORMAT_TIME, &start);
      segment.format = GST_FORMAT_TIME;
      gst_event_unref (event);
      event = gst_event_new_segment (&segment);
    }
  }
  return gst_pad_event_default(mfxpad->pad, (GstObject*)plugin, event);
}

gboolean mfx_gst_plugin_vdec_output_event(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstEvent *event)
{
  MFX_DEBUG_TRACE_FUNC;
  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);
  MFX_DEBUG_TRACE__GstEventType(event->type);

  return gst_pad_event_default(mfxpad->pad, (GstObject*)plugin, event);
}

GstFlowReturn mfx_gst_vdec_chain(mfx_GstPad *mfxpad, mfx_GstPlugin *plugin, GstBuffer *buffer)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE__GstStaticPadTemplate(mfxpad->templ->gst_templ);

  if (!plugin || !buffer) {
    return GST_FLOW_ERROR;
  }

  if (GST_BUFFER_DTS (buffer) == GST_CLOCK_TIME_NONE &&
      GST_BUFFER_PTS (buffer) == GST_CLOCK_TIME_NONE) {
    GstClock *clock = gst_element_get_clock (&plugin->element);
    if (clock) {
        GstClockTime base_time =
            gst_element_get_base_time (&plugin->element);
        GstClockTime now = gst_clock_get_time (clock);
        if (now > base_time)
          now -= base_time;
        else
          now = 0;
        gst_object_unref (clock);

        GST_BUFFER_PTS (buffer) = now;
        GST_BUFFER_DTS (buffer) = now;
    }
  }
  mfxGstPluginVdecData* vdec = (mfxGstPluginVdecData*)plugin->data;

  std::shared_ptr<mfxGstPluginVdecData::InputData> input_data(new mfxGstPluginVdecData::InputData);
  input_data->bst_ref.reset(new mfxGstBitstreamBufferRef(buffer));
  gst_buffer_unref(buffer);

  return (vdec->PostDecodeTask(input_data))? GST_FLOW_OK: GST_FLOW_ERROR;
}

bool mfx_gst_vdec_get_property(
  mfx_GstPlugin *plugin,
  guint id, GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxGstPluginVdecData* vdec = (mfxGstPluginVdecData*)plugin->data;
  return vdec->GetProperty(id, val, ps);
}

bool mfx_gst_vdec_set_property(
  mfx_GstPlugin *plugin,
  guint id, const GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxGstPluginVdecData* vdec = (mfxGstPluginVdecData*)plugin->data;
  return vdec->SetProperty(id, val, ps);
}

G_END_DECLS
