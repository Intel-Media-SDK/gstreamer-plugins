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
#include "mfx_gst_video_frame_pool.h"

#include "vaapi_allocator.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgst_video_framepool"

G_BEGIN_DECLS

struct GstMfxVideoFramePoolClass
{
  GstBufferPoolClass base_class;
};

struct GstMfxVideoFramePool
{
  GstBufferPool parent_instance;
  GstAllocator* allocator;
};

static GstFlowReturn
gst_mfx_video_frame_pool_alloc_buffer(
  GstBufferPool * pool,
  GstBuffer ** out_buffer_ptr,
  GstBufferPoolAcquireParams * params)
{
  MFX_DEBUG_TRACE_FUNC;
  GstBuffer* buffer = NULL;
  GstMfxVideoFramePool* mfx_pool = (GstMfxVideoFramePool*)pool;

  buffer = gst_buffer_new();
  if (buffer) {
    gsize size = 0;

    GstMfxFrameSurfaceMemory *mem = NULL;

    mem = g_slice_new0(GstMfxFrameSurfaceMemory);
    if (!mem)
      return GST_FLOW_ERROR;

    gst_memory_init(GST_MEMORY_CAST(mem), GST_MEMORY_FLAG_NO_SHARE,
                    mfx_pool->allocator, NULL, size, 0, 0, size);
    gst_buffer_append_memory(buffer, GST_MEMORY_CAST(mem));

    GstMfxMemoryAllocator* allocator = (GstMfxMemoryAllocator*)mfx_pool->allocator;
    mfxFrameAllocRequest request = allocator->request;

    memcpy(&mem->mfx_surface.Info, &(request.Info), sizeof(mfxFrameInfo));
    mem->mfx_surface.Data.MemId = allocator->response.mids[allocator->num_frames_used++];
  }

  *out_buffer_ptr = buffer;
  return GST_FLOW_OK;
}

static void
gst_mfx_video_frame_pool_finalize(GObject * object)
{
  GstMfxVideoFramePool * const pool = (GstMfxVideoFramePool*)object;

  gst_object_unref(pool->allocator);

  GstBufferPoolClass *base_class =
    (GstBufferPoolClass *)g_type_class_ref(GST_TYPE_BUFFER_POOL);
  GObjectClass * const base_object_class = G_OBJECT_CLASS(base_class);

   base_object_class->finalize(object);
}


static void gst_mfx_video_frame_pool_class_intern_init(gpointer klass, gpointer /*class_data*/)
{
  GObjectClass * const object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = gst_mfx_video_frame_pool_finalize;

  GstBufferPoolClass* pool_class = (GstBufferPoolClass*)klass;
  pool_class->alloc_buffer = gst_mfx_video_frame_pool_alloc_buffer;
}

static void
gst_mfx_video_frame_pool_init(GTypeInstance *instance, gpointer g_class)
{
}

MfxGstTypeDefinition g_gst_mfx_video_frame_pool_type =
{
  .parent = GST_TYPE_BUFFER_POOL,
  .type_name = g_intern_static_string("GstMfxVideoFramePool"),
  .class_size = sizeof(GstMfxVideoFramePoolClass),
  .class_init = gst_mfx_video_frame_pool_class_intern_init,
  .instance_size = sizeof(GstMfxVideoFramePool),
  .instance_init = gst_mfx_video_frame_pool_init,
  .flags = (GTypeFlags)0,
  .type_id = 0
};

extern MfxGstTypeDefinition gst_mfx_memory_allocator_type;

GstBufferPool *
gst_mfx_video_frame_pool_new(MFXFrameAllocator* mfx_allocator, mfxFrameAllocRequest* request)
{
  MFX_DEBUG_TRACE_FUNC;
  GstMfxVideoFramePool* pool;
  GstMfxMemoryAllocator* allocator;

  pool = (GstMfxVideoFramePool*)g_object_new(mfx_gst_get_type(g_gst_mfx_video_frame_pool_type), NULL);
  if (!pool) {
    return NULL;
  }

  allocator = (GstMfxMemoryAllocator*)g_object_new(mfx_gst_get_type(gst_mfx_memory_allocator_type), NULL);
  if (!allocator) {
    return NULL;
  }

  allocator->mfx_allocator = mfx_allocator;
  if (request) {
    memcpy(&allocator->request, request, sizeof(mfxFrameAllocRequest));
  }

  mfxStatus sts = mfx_allocator->Alloc(mfx_allocator->pthis, &allocator->request, &allocator->response);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_MSG("MFX allocator failed to allocate frame");
    return NULL;
  }
  pool->allocator = GST_ALLOCATOR_CAST(allocator);

  return (GstBufferPool*)pool;
}

G_END_DECLS
