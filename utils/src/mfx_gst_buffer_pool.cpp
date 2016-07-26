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
#include "mfx_gst_buffer_pool.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstbufferpool"

G_BEGIN_DECLS

struct GstMfxBufferPoolClass
{
  GstBufferPoolClass base_class;
};

struct GstMfxBufferPool
{
  GstBufferPool parent_instance;
  gsize init_buffers_size;
};

static GstFlowReturn
gst_mfx_buffer_pool_alloc_buffer(
  GstBufferPool * pool,
  GstBuffer ** out_buffer_ptr,
  GstBufferPoolAcquireParams * params)
{
  GstMfxBufferPool* mfx_pool = (GstMfxBufferPool*)pool;
  GstBufferPoolClass *base_pool_class =
    (GstBufferPoolClass *)g_type_class_ref(GST_TYPE_BUFFER_POOL);
  GstFlowReturn result = base_pool_class->alloc_buffer(pool, out_buffer_ptr, params);

  if ((result == GST_FLOW_OK) && out_buffer_ptr) {
    mfx_pool->init_buffers_size = gst_buffer_get_size(*out_buffer_ptr);
  }
  return result;
}

static void
gst_mfx_buffer_pool_reset_buffer (GstBufferPool * pool,
    GstBuffer * buffer)
{
  GstMfxBufferPool* mfx_pool = (GstMfxBufferPool*)pool;
  GstBufferPoolClass *base_pool_class =
    (GstBufferPoolClass *)g_type_class_ref(GST_TYPE_BUFFER_POOL);

  gst_buffer_set_size(buffer, mfx_pool->init_buffers_size);
  base_pool_class->reset_buffer(pool, buffer);
}

static void gst_mfx_buffer_pool_class_intern_init(gpointer klass, gpointer /*class_data*/)
{
  GstBufferPoolClass* pool_class = (GstBufferPoolClass*)klass;
  pool_class->alloc_buffer = gst_mfx_buffer_pool_alloc_buffer;
  pool_class->reset_buffer = gst_mfx_buffer_pool_reset_buffer;
}

static void
gst_mfx_buffer_pool_init(GTypeInstance *instance, gpointer g_class)
{
  //GstMfxBufferPool* klass = (GstMfxBufferPool*)instance;
}

MfxGstTypeDefinition g_mfx_buffer_pool_type =
{
  .parent = GST_TYPE_BUFFER_POOL,
  .type_name = g_intern_static_string("GstMfxBufferPool"),
  .class_size = sizeof(GstMfxBufferPoolClass),
  .class_init = gst_mfx_buffer_pool_class_intern_init,
  .instance_size = sizeof(GstMfxBufferPool),
  .instance_init = gst_mfx_buffer_pool_init,
  .flags = (GTypeFlags)0,
  .type_id = 0
};

GstBufferPool *
gst_mfx_buffer_pool_new (void)
{
  return (GstBufferPool*)g_object_new(mfx_gst_get_type(g_mfx_buffer_pool_type), NULL);
}

G_END_DECLS
