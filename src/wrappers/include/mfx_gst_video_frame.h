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

#ifndef __MFX_GST_VIDEO_FRAME_H__
#define __MFX_GST_VIDEO_FRAME_H__

#include "base_allocator.h"
#include "vaapi_allocator.h"
#include "mfx_wrappers.h"
#include "mfx_gst_debug.h"
#include "mfx_gst_buffer.h"
#include "mfx_utils.h"
#include "mfx_gst_video_frame_pool.h"

struct ImfxGstVideoRef {
  virtual ~ImfxGstVideoRef() {}
  virtual mfxFrameSurface1* srf() = 0;
  virtual mfxFrameInfo* info() = 0;
};

struct mfxGstVideoBase : public ImfxGstVideoRef, public mfxGstBufferRef {
  mfxGstVideoBase(GstBuffer* buffer) :
    mfxGstBufferRef(buffer)
  {
  }
  virtual ~mfxGstVideoBase() {}
};

struct mfxGstSystemMemoryFrame : public mfxGstVideoBase {
  mfxGstSystemMemoryFrame(GstBuffer* buffer, GstVideoInfo * const info)
    : mfxGstVideoBase(buffer)
    , info_(info)

  {
    MSDK_ZERO_MEM(srf_);
    MSDK_ZERO_MEM(aligned_srf_);
  }
  ~mfxGstSystemMemoryFrame()
  {
    unmap();
  }

  // Returns mediasdk surface representation wrapped by this object.
  // Function may return NULL if map() operation failed.
  mfxFrameSurface1* srf();

  mfxFrameInfo* info() {
    if (aligned_srf_.Data.Y) return &aligned_srf_.Info;
    return &srf_.Info;
  }

protected:
  virtual bool map(GstMapFlags purpose);
  virtual void unmap();

protected:
  mfxFrameSurface1 srf_;
  MfxFrameSurface1Wrap aligned_srf_;
  GstVideoInfo * const info_;
};

struct mfxGstVideoMemoryFrame : public mfxGstVideoBase {
  mfxGstVideoMemoryFrame(GstBuffer* buffer, GstVideoInfo * const info)
    : mfxGstVideoBase(buffer)
    , srf_(NULL)
    , info_(info)
  {
  }
  virtual ~mfxGstVideoMemoryFrame()
  {
    unmap();
  }

  virtual mfxFrameSurface1* srf() {
    if (!srf_ && !map((GstMapFlags) (GST_MAP_READ | GST_MAP_WRITE))) return NULL;
    return srf_;
  }

  virtual mfxFrameInfo* info() {
    mfxFrameSurface1 * srf = NULL;
    GstMemory* mem = gst_buffer_peek_memory(buffer_, 0);
    if (mem) {
      GstMfxFrameSurfaceMemory* mfxmem = (GstMfxFrameSurfaceMemory*) mem;
      if (mfxmem) {
        srf = &mfxmem->mfx_surface;
      }
    }
    return srf ? &srf->Info : NULL;
  }

protected:
  virtual bool map(GstMapFlags purpose) {
    GstMemory* mem = gst_buffer_peek_memory(buffer_, 0);
    if (mem) {
      GstMfxFrameSurfaceMemory* mfxmem = (GstMfxFrameSurfaceMemory*) mem;
      if (mfxmem) {
        srf_ = &mfxmem->mfx_surface;
        if (srf_) {
          srf_->Data.TimeStamp = GST_BUFFER_TIMESTAMP(buffer_);
          srf_->Info.PicStruct = get_mfx_picstruct(buffer_, info_ ? info_->interlace_mode : GST_VIDEO_INTERLACE_MODE_PROGRESSIVE);
        }
      }
    }
    return srf_ ? true : false;
  }
  virtual void unmap() {
    if (srf_) {
      GST_BUFFER_TIMESTAMP(buffer_) = srf_->Data.TimeStamp;
      mfxU32 flags = get_gst_picstruct(srf_->Info.PicStruct);
      GST_BUFFER_FLAG_SET(buffer_, flags);
    }
  }

  mfxFrameSurface1 * srf_;
  GstVideoInfo * const info_;
};

struct mfxGstDMAMemoryFrame : public mfxGstVideoBase {
  mfxGstDMAMemoryFrame(GstBuffer* buffer, GstVideoInfo * const info, MFXFrameAllocator* allocator)
    : mfxGstVideoBase(buffer)
    , allocator_(allocator)
    , info_(info)
  {
    MSDK_ZERO_MEM(srf_);
  }
  virtual ~mfxGstDMAMemoryFrame()
  {
    unmap();
  }

  virtual mfxFrameSurface1* srf() {
    if (!srf_.Data.MemId && !map((GstMapFlags) (GST_MAP_READ | GST_MAP_WRITE))) return NULL;
    return &srf_;
  }

  virtual mfxFrameInfo* info() {
    return &srf_.Info;
  }

protected:
  virtual bool map(GstMapFlags purpose);
  virtual void unmap();

  MFXFrameAllocator* allocator_;
  mfxFrameSurface1 srf_;
  GstVideoInfo * const info_;
};

struct mfxGstVideoFrameRef : public ImfxGstVideoRef {
  mfxGstVideoFrameRef(GstBuffer* buffer, GstVideoInfo * const info = NULL, MFXFrameAllocator* allocator = NULL)
    {
    if (isDMABuffer(buffer)) {
      obj_.reset(new mfxGstDMAMemoryFrame(buffer, info, allocator));
    }
    else if (isMfxSurface(buffer)) {
      obj_.reset(new mfxGstVideoMemoryFrame(buffer, info));
    }
    else {
      obj_.reset(new mfxGstSystemMemoryFrame(buffer, info));
    }
  }

  virtual ~mfxGstVideoFrameRef() {
      obj_.reset();
  }

  virtual mfxFrameSurface1* srf() {
    return obj_->srf();
  }

  virtual GstBuffer* getBuffer() {
    return obj_->getBuffer();
  }

  virtual mfxFrameInfo* info() {
    return obj_->info();
  }

  inline mfxSyncPoint* syncp() {
    return &syncp_;
  }

private:
  inline bool isMfxSurface(GstBuffer* buffer) {
    if (1 != gst_buffer_n_memory(buffer)) {
      return false;
    }
    GstMemory* mem = gst_buffer_peek_memory(buffer, 0);
    if (!mem) {
      return false;
    }
    if (!gst_memory_is_type(mem, GST_MFX_FRAME_SURFACE_MEMORY_NAME)) {
      return false;
    }
    return true;
  }
  inline bool isDMABuffer(GstBuffer* buffer) {
    if (1 != gst_buffer_n_memory(buffer)) {
      return false;
    }
    GstMemory* mem = gst_buffer_peek_memory(buffer, 0);
    if (!mem) {
      return false;
    }
    if (gst_is_dmabuf_memory(mem)) {
      return true;
    }
    return false;
  }

protected:
   std::unique_ptr<mfxGstVideoBase> obj_;
   mfxSyncPoint syncp_;
};

class mfxGstVideoFramePoolWrap
{
public:
  mfxGstVideoFramePoolWrap()
    : pool_(NULL)
  {}
  ~mfxGstVideoFramePoolWrap() { Close(); }

  bool Init(MFXFrameAllocator* allocator, mfxFrameAllocRequest* request, GstVideoInfo * info = 0);
  void Close();

  std::shared_ptr<mfxGstVideoFrameRef> GetBuffer();

protected:
  GstBufferPool *pool_;
  GstVideoInfo * info_;

private:
  mfxGstVideoFramePoolWrap(const mfxGstVideoFramePoolWrap&);
  mfxGstVideoFramePoolWrap& operator=(const mfxGstVideoFramePoolWrap&);
};

#endif
