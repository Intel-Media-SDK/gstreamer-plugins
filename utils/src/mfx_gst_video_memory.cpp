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

#include "base_allocator.h"
#include "mfx_gst_video_memory.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgst_videomemory"

static GstMemory *
gst_mfx_surface_memory_allocator_alloc(GstAllocator* allocator, gsize size,
    GstAllocationParams* params)
{
  g_assert_not_reached();
  return NULL;
}

gpointer gst_mfx_surface_memory_allocator_map(GstMemory * mem, gsize maxsize, GstMapFlags flags)
{
  g_assert_not_reached();
  return NULL;
}

void gst_mfx_surface_memory_allocator_unmap(GstMemory *mem)
{
  g_assert_not_reached();
  return;
}

static void
gst_mfx_surface_memory_allocator_free(GstAllocator* allocator,
                                      GstMemory* mem)
{
  GstMfxFrameSurfaceMemory *mfxmem = (GstMfxFrameSurfaceMemory*)mem;
  g_slice_free(GstMfxFrameSurfaceMemory, mfxmem);
}

static void
gst_mfx_memory_allocator_finalize(GObject * object)
{
  GstMfxMemoryAllocator * const allocator = (GstMfxMemoryAllocator*)object;
  if (allocator) {
    MFXFrameAllocator* mfx_allocator = allocator->mfx_allocator;
    mfx_allocator->Free(mfx_allocator->pthis, &allocator->response);
  }

  GstAllocatorClass *base_class =
    (GstAllocatorClass *)g_type_class_ref(GST_TYPE_ALLOCATOR);
  GObjectClass * const base_object_class = G_OBJECT_CLASS(base_class);

  base_object_class->finalize(object);
}

static void
gst_mfx_memory_allocator_class_init(gpointer klass, gpointer /*class_data*/)
{
  GObjectClass * const object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = gst_mfx_memory_allocator_finalize;

  GstAllocatorClass * allocator_class = (GstAllocatorClass *)klass;
  allocator_class->alloc = gst_mfx_surface_memory_allocator_alloc;
  allocator_class->free = gst_mfx_surface_memory_allocator_free;
}

static void
gst_mfx_memory_allocator_init(GTypeInstance *instance, gpointer /*g_class*/)
{
  GstAllocator *alloc = (GstAllocator*)instance;

  alloc->mem_type = GST_MFX_FRAME_SURFACE_MEMORY_NAME;
  alloc->mem_map = gst_mfx_surface_memory_allocator_map;
  alloc->mem_unmap = gst_mfx_surface_memory_allocator_unmap;
}

MfxGstTypeDefinition gst_mfx_memory_allocator_type =
{
  .parent = GST_TYPE_ALLOCATOR,
  .type_name = g_intern_static_string("GstMfxMemoryAllocator"),
  .class_size = sizeof(GstMfxMemoryAllocatorClass),
  .class_init = gst_mfx_memory_allocator_class_init,
  .instance_size = sizeof(GstMfxMemoryAllocator),
  .instance_init = gst_mfx_memory_allocator_init,
  .flags = (GTypeFlags)0,
  .type_id = 0
};
