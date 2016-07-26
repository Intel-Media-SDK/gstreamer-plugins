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

#ifndef __MFX_GST_PLUGIN_VDEC_H__
#define __MFX_GST_PLUGIN_VDEC_H__

#include "mfx_gst_plugin.h"

G_BEGIN_DECLS

extern gpointer mfx_gst_plugin_vdec_data_init(mfx_GstPlugin* plugin);
extern gpointer mfx_gst_plugin_vdec_data_dispose(mfx_GstPlugin* plugin);

extern GstStateChangeReturn mfx_gst_plugin_vdec_change_state(
  mfx_GstPlugin *plugin,
  GstStateChange state);

extern gboolean mfx_gst_vdec_query(mfx_GstPad *pad, mfx_GstPlugin *plugin, GstQuery *query);
extern gboolean mfx_gst_plugin_vdec_input_event(mfx_GstPad *pad, mfx_GstPlugin *plugin, GstEvent *data);
extern gboolean mfx_gst_plugin_vdec_output_event(mfx_GstPad *pad, mfx_GstPlugin *plugin, GstEvent *data);
extern GstFlowReturn mfx_gst_vdec_chain(mfx_GstPad *pad, mfx_GstPlugin *plugin, GstBuffer *data);
extern bool mfx_gst_vdec_get_property(
  mfx_GstPlugin *plugin,
  guint id, GValue *val, GParamSpec *ps);
extern bool mfx_gst_vdec_set_property(
  mfx_GstPlugin *plugin,
  guint id, const GValue *val, GParamSpec *ps);

G_END_DECLS

#endif
