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

#include "mfx_defs.h"
#include "mfx_gst_debug.h"
#include "mfx_gst_utils.h"
#include "mfx_vaapi_debug.h"
#include "mfx_gst_context.h"

#include <fcntl.h>
#include <unistd.h>

G_BEGIN_DECLS

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstplugin_context"

struct _GstMfxVaDisplay
{
  GObject gobject;
  int drmfd;
  VADisplay vadpy;
};

struct GstMfxVaDisplayClass
{
  GObjectClass gobject_class;
};

static void
gst_mfx_display_instance_init(GTypeInstance *instance, gpointer g_class)
{
  MFX_DEBUG_TRACE_FUNC;
  GstMfxVaDisplay* obj = (GstMfxVaDisplay*)instance;

  int major_version, minor_version;
  VAStatus va_sts;

  obj->drmfd = open(MSDK_PRIMARY_VIDEOCARD, O_RDWR);
  if (obj->drmfd < 0) {
    MFX_DEBUG_TRACE_MSG("failed to open primary videocard");
    goto _error;
  }

  obj->vadpy = vaGetDisplayDRM(obj->drmfd);
  va_sts = vaInitialize(obj->vadpy, &major_version, &minor_version);
  if (VA_STATUS_SUCCESS != va_sts) {
    MFX_DEBUG_TRACE_MSG("failed to initialize LibVA");
    goto _error;
  }

  return;

  _error:
  if (obj->vadpy) {
    vaTerminate(obj->vadpy);
    obj->vadpy = NULL;
  }
  if (obj->drmfd >=0) {
    close(obj->drmfd);
    obj->drmfd = -1;
  }
}

void gst_mfx_display_finalize(GObject *object)
{
  GstMfxVaDisplay* obj = (GstMfxVaDisplay*)object;
  if (obj->vadpy) {
    vaTerminate(obj->vadpy);
    obj->vadpy = NULL;
  }
  if (obj->drmfd >=0) {
    close(obj->drmfd);
    obj->drmfd = -1;
  }
}

static void gst_mfx_display_class_init(gpointer klass, gpointer /*class_data*/)
{
  GObjectClass* gobject_class = (GObjectClass*)klass;
  gobject_class->finalize = gst_mfx_display_finalize;
}

/**************************************************************************************/

static gboolean
mfx_gst_pad_peer_query_func(const GValue * item, GValue * ret, gpointer user_data)
{
  gboolean cont = true;
  GstPad * const pad = (GstPad*)g_value_get_object(item);
  GstQuery * const query = (GstQuery*)user_data;

  if (gst_pad_peer_query(pad, query)) {
    g_value_set_boolean(ret, true);
    cont = false;
  }
  return cont;
}

static gboolean
mfx_gst_query_peers(GstElement * element, GstQuery * query,
                    GstPadDirection direction)
{
  GstIterator *iter;
  GValue res = { 0 };

  g_value_init(&res, G_TYPE_BOOLEAN);
  g_value_set_boolean(&res, false);

  if (direction == GST_PAD_SRC)
    iter = gst_element_iterate_src_pads(element);
  else
    iter = gst_element_iterate_sink_pads(element);

  GstIteratorFoldFunction func = mfx_gst_pad_peer_query_func;
  while (GST_ITERATOR_RESYNC == gst_iterator_fold(iter, func, &res, query)) {
    gst_iterator_resync(iter);
  }
  gst_iterator_free(iter);

  return g_value_get_boolean(&res);
}

static GstContext*
mfx_gst_get_context_from_peers(GstElement * element, GstQuery * query,
                            GstPadDirection direction)
{
  GstContext * ctx = NULL;

  if (!mfx_gst_query_peers(element, query, direction))
    return NULL;

  gst_query_parse_context(query, &ctx);

  gst_element_set_context(element, ctx);
  return ctx;
}

#define MFX_GST_VA_DISPLAY_TYPE mfx_gst_get_type(g_mfx_va_display_type)

MfxGstTypeDefinition g_mfx_va_display_type =
{
  .parent = G_TYPE_OBJECT,
  .type_name = g_intern_static_string("GstMfxVaDisplay"),
  .class_size = sizeof(GstMfxVaDisplayClass),
  .class_init = gst_mfx_display_class_init,
  .instance_size = sizeof(GstMfxVaDisplay),
  .instance_init = gst_mfx_display_instance_init,
  .flags = (GTypeFlags)0,
  .type_id = 0
};

GstMfxVaDisplay *
mfx_gst_va_display_new(void)
{
  MFX_DEBUG_TRACE_FUNC;
  return (GstMfxVaDisplay*)g_object_new(MFX_GST_VA_DISPLAY_TYPE, NULL);
}

gboolean mfx_gst_va_display_is_valid(GstMfxVaDisplay* display)
{
  return (display && display->vadpy) ? TRUE : FALSE;
}

VADisplay mfx_gst_va_display_get_VADisplay(GstMfxVaDisplay* display)
{
  return (display) ? display->vadpy : NULL;
}

GstContext*
mfx_gst_query_context(GstElement * element, const gchar * context_type)
{
  GstQuery * query = gst_query_new_context(context_type);
  GstContext * ctx = NULL;

  ctx = mfx_gst_get_context_from_peers(element, query, GST_PAD_SRC);
  if (!ctx) {
    ctx = mfx_gst_get_context_from_peers(element, query, GST_PAD_SINK);
  }

  gst_query_unref(query);
  return ctx;
}

void
mfx_gst_set_contextdata(GstContext *ctx, GstMfxVaDisplay * display)
{
  gst_structure_set(
    gst_context_writable_structure(ctx),
    GST_MFX_VA_DISPLAY_CONTEXT_TYPE,
    MFX_GST_VA_DISPLAY_TYPE, display, NULL);
}

gboolean
mfx_gst_get_contextdata(GstContext *ctx, GstMfxVaDisplay** display)
{
  const GstStructure * structure = NULL;

  if (!ctx) {
    return false;
  }
  if (strcmp(gst_context_get_context_type(ctx), GST_MFX_VA_DISPLAY_CONTEXT_TYPE)) {
    return false;
  }

  structure = gst_context_get_structure(ctx);
  return gst_structure_get (structure, GST_MFX_VA_DISPLAY_CONTEXT_TYPE,
      MFX_GST_VA_DISPLAY_TYPE, display, NULL);
}

gboolean mfx_gst_set_context_in_query(GstQuery * query, GstMfxVaDisplay * display)
{
  MFX_DEBUG_TRACE_FUNC;
  GstContext *ctx = NULL;

  ctx = gst_context_new(GST_MFX_VA_DISPLAY_CONTEXT_TYPE, false);
  if (!ctx) {
    MFX_DEBUG_TRACE_MSG("failed to allocate GstContext");
    return false;
  }

  mfx_gst_set_contextdata(ctx, display);

  gst_query_set_context(query, ctx);
  return true;
}

G_END_DECLS
