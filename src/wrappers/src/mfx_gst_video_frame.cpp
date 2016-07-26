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

#include "mfx_gst_video_frame.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgst_videoframe"

mfxFrameSurface1* mfxGstSystemMemoryFrame::srf()
{
  if (!mem_ && !map(GST_MAP_READ)) return NULL;
  if (aligned_srf_.Data.Y) return &aligned_srf_;
  if (MSDK_IS_ALIGNED(srf_.Info.Width, 16) &&
      MSDK_IS_ALIGNED(srf_.Info.Height, 16)) {
    return &srf_;
  }

  memcpy(&aligned_srf_.Info, &srf_.Info, sizeof(mfxFrameInfo));

  if (MFX_ERR_NONE != aligned_srf_.Alloc(srf_.Info.FourCC, MSDK_ALIGN16(srf_.Info.Width), MSDK_ALIGN16(srf_.Info.Height)))
    return NULL;

  mfx_gst_copy_srf(aligned_srf_, srf_);
  aligned_srf_.Data.TimeStamp = srf_.Data.TimeStamp;

  return &aligned_srf_;
}

bool mfxGstSystemMemoryFrame::map(GstMapFlags purpose)
{
  MFX_DEBUG_TRACE_FUNC;
  bool res = mfxGstBufferRef::map(purpose);
  if (res) {
    switch (srf_.Info.FourCC) {
      case MFX_FOURCC_NV12:
        srf_.Data.Pitch = MSDK_ALIGN4(srf_.Info.Width);
        srf_.Data.Y  = meminfo_.data;
        srf_.Data.UV = meminfo_.data + srf_.Data.Pitch * MSDK_ALIGN2(srf_.Info.Height);
        break;
      case MFX_FOURCC_YV12:
        srf_.Data.Pitch = MSDK_ALIGN4(srf_.Info.Width);
        srf_.Data.Y = meminfo_.data;
        srf_.Data.V = meminfo_.data + srf_.Data.Pitch * MSDK_ALIGN2(srf_.Info.Height);
        srf_.Data.U = srf_.Data.V + MSDK_ALIGN4(MSDK_ALIGN2(srf_.Info.Width)/2) * (MSDK_ALIGN2(srf_.Info.Height)/2);
        break;
      case MFX_FOURCC_I420:
        // here is implicit pointers swap for I420 color format
        srf_.Info.FourCC = MFX_FOURCC_YV12;
        srf_.Data.Y = meminfo_.data;
        srf_.Data.U = meminfo_.data + srf_.Info.Width * srf_.Info.Height;
        srf_.Data.V = srf_.Data.U + (srf_.Info.Width>>1)*(srf_.Info.Height>>1);
        srf_.Data.Pitch = srf_.Info.Width;
        break;
      case MFX_FOURCC_YUY2:
        srf_.Data.Y = meminfo_.data;
        srf_.Data.V = meminfo_.data + 1;
        srf_.Data.U = meminfo_.data + 3;
        srf_.Data.Pitch = MSDK_ALIGN4(2 * srf_.Info.Width);
        break;
      case MFX_FOURCC_UYVY:
        srf_.Data.U = meminfo_.data;
        srf_.Data.Y = meminfo_.data + 1;
        srf_.Data.V = meminfo_.data + 2;
        srf_.Data.Pitch = MSDK_ALIGN4(2 * srf_.Info.Width);
        break;
      case MFX_FOURCC_RGB4:
        srf_.Data.B = meminfo_.data;
        srf_.Data.G = meminfo_.data + 1;
        srf_.Data.R = meminfo_.data + 2;
        srf_.Data.A = meminfo_.data + 3;
        srf_.Data.Pitch = 4 * srf_.Info.Width;
        break;
      default:
        unmap();
        g_assert_not_reached();
        return false;
    }
    srf_.Data.TimeStamp = GST_BUFFER_TIMESTAMP(buffer_);
    srf_.Info.PicStruct = get_mfx_picstruct(buffer_, info_ ? info_->interlace_mode : GST_VIDEO_INTERLACE_MODE_PROGRESSIVE);
  }
  return res;
}

void mfxGstSystemMemoryFrame::unmap()
{
  GST_BUFFER_TIMESTAMP(buffer_) = srf_.Data.TimeStamp;
  mfxU32 flags = get_gst_picstruct(srf_.Info.PicStruct);
  GST_BUFFER_FLAG_SET(buffer_, flags);
  return mfxGstBufferRef::unmap();
}

bool mfxGstDMAMemoryFrame::map(GstMapFlags purpose)
{
  MFX_DEBUG_TRACE_FUNC;
  GstMemory* mem = gst_buffer_peek_memory(buffer_, 0);
  if (mem) {
    gint fd = gst_dmabuf_memory_get_fd(mem);
    if (fd != -1) {
      mfxFrameDMAAllocRequest request;
      MSDK_ZERO_MEM(request);

      memcpy(&request.Info, &srf_.Info, sizeof(mfxFrameInfo));
      request.extbufHandle = fd;

      vaapiFrameAllocator * va_allocator = (vaapiFrameAllocator*)allocator_;
      mfxMemId mid = va_allocator->AllocFromExtHandle(&request);
      if (!mid) {
        MFX_DEBUG_TRACE_MSG("error: can't create vaSurface from DMA handle");
        return false;
      }

      srf_.Data.MemId = mid;
      srf_.Data.TimeStamp = GST_BUFFER_TIMESTAMP(buffer_);
      srf_.Info.PicStruct = get_mfx_picstruct(buffer_, info_ ? info_->interlace_mode : GST_VIDEO_INTERLACE_MODE_PROGRESSIVE);

      // Workaround: pretend to have aligned width/height
      srf_.Info.Width = MSDK_ALIGN16(srf_.Info.Width);
      srf_.Info.Height = MSDK_ALIGN16(srf_.Info.Height);
      return true;
    }
  }
  return false;
}

void mfxGstDMAMemoryFrame::unmap()
{
  if (srf_.Data.MemId) {
    vaapiFrameAllocator * va_allocator = (vaapiFrameAllocator*)allocator_;
    va_allocator->ReleaseExtHandle(srf_.Data.MemId);
    srf_.Data.MemId = NULL;
  }

  GST_BUFFER_TIMESTAMP(buffer_) = srf_.Data.TimeStamp;
  mfxU32 flags = get_gst_picstruct(srf_.Info.PicStruct);
  GST_BUFFER_FLAG_SET(buffer_, flags);
}

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgst_videoframe_bufferpool"

bool mfxGstVideoFramePoolWrap::Init(MFXFrameAllocator* allocator, mfxFrameAllocRequest* request, GstVideoInfo * info)
{
  MFX_DEBUG_TRACE_FUNC;
  GstStructure* config = NULL;
  GstCaps *caps = NULL;
  gboolean res;
  guint buffer_size = 0;

  if (!request) {
    MFX_DEBUG_TRACE_MSG("NULL request");
    return false;
  }

  if (pool_) {
    MFX_DEBUG_TRACE_MSG("pool is already inititialized");
    return false;
  }

  if (request->Type & MFX_MEMTYPE_SYSTEM_MEMORY) {
    pool_ = gst_buffer_pool_new();

    // TODO: we have to implement handling of interlaced cases
    mfxU32 Width2 = MSDK_ALIGN16(request->Info.Width);
    mfxU32 Height2 = MSDK_ALIGN16(request->Info.Height);

    switch (request->Info.FourCC)
    {
    case MFX_FOURCC_YV12:
    case MFX_FOURCC_NV12:
        buffer_size = Width2*Height2 + (Width2>>1)*(Height2>>1) + (Width2>>1)*(Height2>>1);
        break;
    case MFX_FOURCC_NV16:
        buffer_size = Width2*Height2 + (Width2>>1)*(Height2) + (Width2>>1)*(Height2);
        break;
    case MFX_FOURCC_RGB3:
        buffer_size = Width2*Height2 + Width2*Height2 + Width2*Height2;
        break;
    case MFX_FOURCC_RGB4:
        buffer_size = Width2*Height2 + Width2*Height2 + Width2*Height2 + Width2*Height2;
        break;
    case MFX_FOURCC_UYVY:
    case MFX_FOURCC_YUY2:
        buffer_size = Width2*Height2 + (Width2>>1)*(Height2) + (Width2>>1)*(Height2);
        break;
    case MFX_FOURCC_R16:
        buffer_size = 2*Width2*Height2;
        break;
    case MFX_FOURCC_P010:
        buffer_size = Width2*Height2 + (Width2>>1)*(Height2>>1) + (Width2>>1)*(Height2>>1);
        buffer_size *= 2;
        break;
    case MFX_FOURCC_A2RGB10:
        buffer_size = Width2*Height2*4; // 4 bytes per pixel
        break;
    case MFX_FOURCC_P210:
        buffer_size = Width2*Height2 + (Width2>>1)*(Height2) + (Width2>>1)*(Height2);
        buffer_size *= 2; // 16bits
        break;
    default:
        return false;
    }
  }
  else {
    pool_ = gst_mfx_video_frame_pool_new(allocator, request);
  }

  if (!pool_) {
    MFX_DEBUG_TRACE_MSG("failed to allocate the pool");
    goto error;
  }

  config = gst_buffer_pool_get_config(pool_);
  if (!config) {
    MFX_DEBUG_TRACE_MSG("failed to get config structure");
    goto error;
  }

  if (info) {
    caps = gst_video_info_to_caps(info);
    info_ = info;
  }
  if (!caps) {
    caps = gst_caps_new_empty_simple ("test/data"); // FIXME
  }
  gst_buffer_pool_config_set_params(config, caps, buffer_size, request->NumFrameSuggested, request->NumFrameSuggested);

  res = gst_buffer_pool_set_config(pool_, config);
  config = NULL;
  if (res != TRUE) {
    MFX_DEBUG_TRACE_MSG("failed to set configuration of pool");
    goto error;
  }

  gst_caps_unref(caps);
  gst_buffer_pool_set_active(pool_, TRUE);
  return true;

  error:
  if (caps) {
    gst_caps_unref (caps);
  }
  if (config) {
    gst_structure_free(config);
  }
  MSDK_OBJECT_UNREF(pool_);

  return false;
}

void mfxGstVideoFramePoolWrap::Close()
{
  if (pool_) {
    gst_buffer_pool_set_active(pool_, FALSE);
    gst_object_unref(pool_);
    pool_ = NULL;
  }
}

std::shared_ptr<mfxGstVideoFrameRef> mfxGstVideoFramePoolWrap::GetBuffer()
{
  MFX_DEBUG_TRACE_FUNC;
  GstBuffer* buffer = NULL;

  if (GST_FLOW_OK != gst_buffer_pool_acquire_buffer(pool_, &buffer, NULL)) {
    return NULL;
  }

  MFX_DEBUG_TRACE_P(buffer);
  if (!buffer) {
    MFX_DEBUG_TRACE_MSG("failed to acquire buffer from pool");
    return NULL;
  }

  std::shared_ptr<mfxGstVideoFrameRef> buffer_ref = std::make_shared<mfxGstVideoFrameRef>(buffer, info_);

  gst_buffer_unref(buffer);
  return buffer_ref;
}
