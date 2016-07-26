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

#include "mfx_gst_plugin.h"

G_BEGIN_DECLS

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstplugin"

#ifndef __defs__plugin_typename
#define __defs__plugin_typename __defs__plugin_name
#endif

static gboolean mfx_gst_plugin_init(GstPlugin *plugin);

GST_PLUGIN_EXPORT GstPluginDesc gst_plugin_desc =
{
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  __defs__plugin_name,
  __defs__plugin_desc,
  mfx_gst_plugin_init,
  MFX_GST_VERSION,
  MFX_GST_LICENSE,
  MFX_GST_PACKAGE,
  MFX_GST_PACKAGE,
  MFX_GST_URL,
  MFX_GST_RELEASE_DATATIME
};

/* for GLIB objects */

static void mfx_gst_set_property(GObject* obj, guint id, const GValue *val, GParamSpec *ps);
static void mfx_gst_get_property(GObject *obj, guint id, GValue *val, GParamSpec *ps);

static void mfx_gst_base_init(gpointer klass);
static void mfx_gst_class_init(gpointer klass, gpointer data);
static void mfx_gst_instance_init(GTypeInstance *curr_instance, gpointer klass);

static void mfx_gst_plugin_free(GObject *object);

static GstStateChangeReturn mfx_gst_plugin_change_state(
  GstElement *element,
  GstStateChange state);

static gboolean mfx_gst_pad_query(GstPad *pad, GstObject *parent, GstQuery *query);
static gboolean mfx_gst_pad_event(GstPad *pad, GstObject *parent, GstEvent *event);
static GstFlowReturn mfx_gst_pad_chain(GstPad *pad, GstObject *parent, GstBuffer *data);

gboolean mfx_gst_plugin_init(GstPlugin *plugin)
{
  MFX_DEBUG_TRACE_FUNC;
  GTypeInfo gtype_struct;
  GType plugin_type;

  MFX_DEBUG_TRACE_P(plugin);
  if (plugin == NULL) {
    return FALSE;
  }

  /* fill gtype_struct fields*/
  gtype_struct.class_size = sizeof(mfx_GstBaseClass);

  gtype_struct.base_init = mfx_gst_base_init;
  gtype_struct.base_finalize=NULL;

  gtype_struct.class_init = mfx_gst_class_init;
  gtype_struct.class_finalize=NULL;

  gtype_struct.instance_size = sizeof(mfx_GstPlugin);
  gtype_struct.instance_init = mfx_gst_instance_init;

  gtype_struct.n_preallocs = 0;
  gtype_struct.value_table = NULL;

  plugin_type = g_type_register_static(
    GST_TYPE_ELEMENT,
    __defs__plugin_typename,
    &gtype_struct,
    (GTypeFlags)0);

  if (!plugin_type) {
    MFX_DEBUG_TRACE_MSG("g_type_register_static: FAILED");
    return FALSE;
  }

  /* register our plugin within GStreamer elements */
  if (gst_element_register(plugin, __defs__plugin_name,
      __plugin_def.rank, plugin_type) != TRUE ) {
    MFX_DEBUG_TRACE_MSG("gst_element_register: FAILED");
    return FALSE;
  }

  if (__plugin_def.type_find) {
    MFX_DEBUG_TRACE("setting typefind");
    /* section for splitter plugins */
    if (gst_type_find_register(plugin, __plugin_def.type_name, __plugin_def.rank,
        __plugin_def.type_find, __plugin_def.type_ext,
        gst_caps_from_string(__plugin_def.type_caps),
        NULL, NULL) != TRUE) {
      MFX_DEBUG_TRACE_MSG("gst_type_find_register: FAILED");
      return FALSE;
    }
  }

  MFX_DEBUG_TRACE_MSG("OK");
  return TRUE;
}

static void mfx_gst_base_init(gpointer klass)
{
  MFX_DEBUG_TRACE_FUNC;
  mfx_GstBaseClass* cur_class = (mfx_GstBaseClass*)klass;
  GstPadTemplate * pad_template;
  size_t i;

  if (cur_class == NULL) {
    return;
  }

  gst_element_class_set_static_metadata(
    &(cur_class->element),
    __plugin_def.longname,
    __plugin_def.classification,
    __plugin_def.description,
    __plugin_def.author);

  for (i = 0; i < __plugin_def.pad_templ_n; ++i) {
    MFX_DEBUG_TRACE_I32(i);
    MFX_DEBUG_TRACE__GstStaticPadTemplate(__plugin_def.pad_templ[i].gst_templ);

    pad_template = gst_static_pad_template_get(&(__plugin_def.pad_templ[i].gst_templ));
    gst_element_class_add_pad_template(&(cur_class->element), pad_template);
//    cur_class->pad_templ[i] = pad_template;
  }

  cur_class->element.change_state = mfx_gst_plugin_change_state;
}

static void mfx_gst_class_init(gpointer klass, gpointer )
{
  MFX_DEBUG_TRACE_FUNC;
  GObjectClass* cur_class = (GObjectClass*)klass;

  cur_class->dispose = mfx_gst_plugin_free;
  cur_class->set_property = mfx_gst_set_property;
  cur_class->get_property = mfx_gst_get_property;

  for (size_t i=0; i < __plugin_def.properties_n; ++i) {
    g_object_class_install_property(
      cur_class,
      __plugin_def.properties[i].id,
      MfxGstPluginProperty::get_param_spec(&__plugin_def.properties[i]));
  }
}

static void mfx_gst_instance_init(GTypeInstance *curr_instance, gpointer klass)
{
  MFX_DEBUG_TRACE_FUNC;
  mfx_GstBaseClass* cur_class = (mfx_GstBaseClass*)klass;
  mfx_GstPlugin* cur_plugin = (mfx_GstPlugin*)curr_instance;
  GstPad* pad;
  mfx_GstPad* mfx_pad;
  size_t i;

  if ( (cur_class == NULL) || (cur_plugin == NULL))
    return;

  for (i = 0; i < __plugin_def.pad_templ_n; ++i) {
    MFX_DEBUG_TRACE_I32(i);
    MFX_DEBUG_TRACE__GstStaticPadTemplate(__plugin_def.pad_templ[i].gst_templ);

    if (__plugin_def.pad_templ[i].gst_templ.presence == GST_PAD_ALWAYS) {
      MFX_DEBUG_TRACE("GST_PAD_ALWAYS");
      pad = gst_pad_new_from_static_template( // NOTE was gst_pad_new_from_template
        &(__plugin_def.pad_templ[i].gst_templ),
        __plugin_def.pad_templ[i].gst_templ.name_template);

      /* setup sink pad */
      // TODO should I combine setting of sink and src pad functions?
      if (__plugin_def.pad_templ[i].gst_templ.direction == GST_PAD_SINK) {
        MFX_DEBUG_TRACE("GST_PAD_SINK");
        gst_pad_set_chain_function(pad, mfx_gst_pad_chain);
        gst_pad_set_event_function(pad, mfx_gst_pad_event);
        gst_pad_set_query_function(pad, mfx_gst_pad_query);

        //gst_pad_set_activate_function(pad, mfx_gst_pad_activate);
        //gst_pad_set_activatepull_function(pad, mfx_gst_pad_activatepull);

        //gst_pad_use_fixed_caps(out_pad); // TODO video? why?
        //gst_pad_set_getcaps_function(out_pad, ipp_pad_get_caps); // TODO
      }

      /* setup src pad */
      if (__plugin_def.pad_templ[i].gst_templ.direction == GST_PAD_SRC) {
        MFX_DEBUG_TRACE("GST_PAD_SRC");
        //gst_pad_set_query_type_function(pad, ipp_pad_query_type); // TODO deprecated?
        gst_pad_set_query_function(pad, mfx_gst_pad_query);

        gst_pad_set_event_function(pad, mfx_gst_pad_event);
      }
      mfx_pad = (mfx_GstPad*)malloc(sizeof(mfx_GstPad));
      if (mfx_pad) {
        mfx_pad->pad = pad;
        mfx_pad->templ = &__plugin_def.pad_templ[i];
        cur_plugin->pads = g_list_append(cur_plugin->pads, mfx_pad);
      }

      /* register element */
      gst_element_add_pad(&(cur_plugin->element), pad);
    }
  }

  cur_plugin->props = __plugin_def.properties;
  cur_plugin->props_n = __plugin_def.properties_n;

  cur_plugin->codec_inited = false;
  cur_plugin->data = __plugin_def.data_init(cur_plugin);
}

static void mfx_gst_plugin_free(GObject *object)
{
  MFX_DEBUG_TRACE_FUNC;
  mfx_GstPlugin* plugin = (mfx_GstPlugin*)object;
  GObjectClass *element_class;

  MFX_DEBUG_TRACE_P(object);

  plugin->data = __plugin_def.data_dispose(plugin);

  GList* list = plugin->pads;
  if (list) {
    do {
      if (list->data) free(list->data);
      list = list->next;
    } while(list);
    g_list_free(plugin->pads);
  }
  element_class = (GObjectClass *)g_type_class_ref(GST_TYPE_ELEMENT);
  element_class->dispose(object);
}

static mfx_GstPad* mfx_gst_plugin_get_pad(
  mfx_GstPlugin* plugin,
  GstPad* pad)
{
  //MFX_DEBUG_TRACE_FUNC;
  GList* list;
  mfx_GstPad* mfx_pad;

  if (!plugin) return NULL;
  for (list = plugin->pads; list; list = list->next) {
    mfx_pad = MFX_GST_GET_PAD(list);
    if (mfx_pad && (mfx_pad->pad == pad)) {
      return mfx_pad;
    }
  }
  return NULL;
}

mfx_GstPad* mfx_gst_plugin_get_pad_by_direction(
  mfx_GstPlugin* plugin,
  GstPadDirection direction)
{
  //MFX_DEBUG_TRACE_FUNC;
  GList* list;
  mfx_GstPad* mfx_pad;

  if (!plugin) return NULL;
  for (list = plugin->pads; list; list = list->next) {
    mfx_pad = MFX_GST_GET_PAD(list);
    if (mfx_pad && (mfx_pad->templ->gst_templ.direction == direction)) {
      return mfx_pad;
    }
  }
  return NULL;
}

GstPad* mfx_gst_plugin_get_src_pad(
  mfx_GstPlugin* plugin)
{
  mfx_GstPad* mfx_pad = mfx_gst_plugin_get_pad_by_direction(plugin, GST_PAD_SRC);
  return mfx_pad ? mfx_pad->pad : NULL;
}

GstPad* mfx_gst_plugin_get_sink_pad(
  mfx_GstPlugin* plugin)
{
  mfx_GstPad* mfx_pad = mfx_gst_plugin_get_pad_by_direction(plugin, GST_PAD_SINK);
  return mfx_pad ? mfx_pad->pad : NULL;
}

inline mfx_GstPadFunctions* mfx_gst_plugin_get_pad_functions(mfx_GstPad* pad)
{
  return (pad)? pad->templ->funcs: NULL;
}

static GstFlowReturn mfx_gst_pad_chain(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
  MFX_DEBUG_TRACE_FUNC;
  mfx_GstPlugin* plugin = (mfx_GstPlugin*)parent;
  mfx_GstPad* mfx_pad = mfx_gst_plugin_get_pad(plugin, pad);
  mfx_GstPadFunctions* funcs = mfx_gst_plugin_get_pad_functions(mfx_pad);

  MFX_DEBUG_TRACE_P(pad);
  MFX_DEBUG_TRACE_P(parent);
  MFX_DEBUG_TRACE_P(buffer);

  GstFlowReturn gf_ret = (funcs && funcs->chain)? funcs->chain(mfx_pad, plugin, buffer): GST_FLOW_ERROR;
  MFX_DEBUG_TRACE__GstFlowReturn(gf_ret);
  return gf_ret;
}

static gboolean mfx_gst_pad_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
  MFX_DEBUG_TRACE_FUNC;

  if (!event) return FALSE;

  mfx_GstPlugin* plugin = (mfx_GstPlugin*)parent;
  mfx_GstPad* mfx_pad = mfx_gst_plugin_get_pad(plugin, pad);
  mfx_GstPadFunctions* funcs = mfx_gst_plugin_get_pad_functions(mfx_pad);

  MFX_DEBUG_TRACE_P(pad);
  MFX_DEBUG_TRACE_P(parent);
  MFX_DEBUG_TRACE_P(event);

  gboolean gb_ret = (funcs && funcs->event)? funcs->event(mfx_pad, plugin, event): false;
  MFX_DEBUG_TRACE__gboolean(gb_ret);
  return gb_ret;
}

static gboolean mfx_gst_pad_query(GstPad *pad, GstObject *parent, GstQuery *query)
{
  MFX_DEBUG_TRACE_FUNC;
  mfx_GstPlugin* plugin = (mfx_GstPlugin*)parent;
  mfx_GstPad* mfx_pad = mfx_gst_plugin_get_pad(plugin, pad);
  mfx_GstPadFunctions* funcs = mfx_gst_plugin_get_pad_functions(mfx_pad);

  MFX_DEBUG_TRACE_P(pad);
  MFX_DEBUG_TRACE_P(parent);
  MFX_DEBUG_TRACE_P(query);

  gboolean gb_ret = (funcs && funcs->query)? funcs->query(mfx_pad, plugin, query): false;
  MFX_DEBUG_TRACE__gboolean(gb_ret);
  return gb_ret;
}

static GstStateChangeReturn mfx_gst_plugin_change_state(
  GstElement *element,
  GstStateChange state)
{
  MFX_DEBUG_TRACE_FUNC;
  return __plugin_def.change_state((mfx_GstPlugin*)element, state);
}

static void mfx_gst_get_property(
  GObject *obj,
  guint id, GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;
  if (!__plugin_def.get_property ||
      !__plugin_def.get_property((mfx_GstPlugin*)obj, id, val, ps)) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, ps);
  }
}

static void mfx_gst_set_property(GObject *obj, guint id, const GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;
  if (!__plugin_def.set_property ||
      !__plugin_def.set_property((mfx_GstPlugin*)obj, id, val, ps)) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, ps);
  }
}

G_END_DECLS
