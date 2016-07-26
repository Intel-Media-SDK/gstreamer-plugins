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

#ifndef __MFX_WRAPPERS_H__
#define __MFX_WRAPPERS_H__

#include <string.h>

#include "mfx_defs.h"
#include "mfx_mfx_debug.h"
#include "mfx_utils.h"
#include "mfxstructures.h"

struct MfxBistreamWrap: public mfxBitstream
{
  MfxBistreamWrap()
  {
    memset((mfxBitstream*)this, 0, sizeof(mfxBitstream));
  }
};

struct MfxFrameSurface1Wrap: public mfxFrameSurface1
{
  MfxFrameSurface1Wrap()
    : buffer_(NULL)
  {
    memset((mfxFrameSurface1*)this, 0, sizeof(mfxFrameSurface1));
  }
  ~MfxFrameSurface1Wrap()
  {
    MSDK_FREE(buffer_);
  }

  mfxStatus Alloc(mfxU32 fourcc, mfxU32 width, mfxU32 height)
  {
    MFX_DEBUG_TRACE_FUNC;

    mfxFrameSurface1 & srf = *this;
    mfxU32 pitch = 0;
    mfxU32 buffer_size = 0;
    srf.Info.Height = height;
    srf.Info.Width = width;

    switch (fourcc)
    {
    case MFX_FOURCC_NV12:
      pitch = width;
      srf.Data.PitchHigh = (mfxU16)(pitch >> 16);
      srf.Data.PitchLow = (mfxU16)pitch;

      buffer_size = 3 * pitch * height / 2;
      buffer_ = (mfxU8 *)calloc(1, buffer_size);
      if (!buffer_) return MFX_ERR_MEMORY_ALLOC;

      srf.Data.Y = buffer_;
      srf.Data.UV = srf.Data.Y + pitch*srf.Info.Height;
      break;
    case MFX_FOURCC_YV12:
      pitch = width;
      srf.Data.PitchHigh = (mfxU16)(pitch >> 16);
      srf.Data.PitchLow = (mfxU16)pitch;

      buffer_size = width*pitch + (pitch>>1)*(height>>1) + (pitch>>1)*(height>>1);
      buffer_ = (mfxU8 *)calloc(1, buffer_size);
      if (!buffer_) return MFX_ERR_MEMORY_ALLOC;

      srf.Data.Y = buffer_;
      srf.Data.V = srf.Data.Y + pitch*height;
      srf.Data.U = srf.Data.V + (pitch>>1)*(height>>1);
      break;
    case MFX_FOURCC_YUY2:
      pitch = width*2;
      srf.Data.PitchHigh = (mfxU16)(pitch >> 16);
      srf.Data.PitchLow = (mfxU16)pitch;

      buffer_size = pitch*height;
      buffer_ = (mfxU8 *)calloc(1, buffer_size);
      if (!buffer_) return MFX_ERR_MEMORY_ALLOC;

      srf.Data.Y = buffer_;
      srf.Data.V = srf.Data.Y + 1;
      srf.Data.U = srf.Data.Y + 3;
      break;
    case MFX_FOURCC_UYVY:
      pitch = width*2;
      srf.Data.PitchHigh = (mfxU16)(pitch >> 16);
      srf.Data.PitchLow = (mfxU16)pitch;

      buffer_size = pitch*height;
      buffer_ = (mfxU8 *)calloc(1, buffer_size);
      if (!buffer_) return MFX_ERR_MEMORY_ALLOC;

      srf.Data.U = buffer_;
      srf.Data.Y = srf.Data.U + 1;
      srf.Data.V = srf.Data.U + 2;
      break;
    case MFX_FOURCC_RGB4:
      pitch = width*4;
      srf.Data.PitchHigh = (mfxU16)(pitch >> 16);
      srf.Data.PitchLow = (mfxU16)pitch;

      buffer_size = pitch*height;
      buffer_ = (mfxU8 *)calloc(1, buffer_size);
      if (!buffer_) return MFX_ERR_MEMORY_ALLOC;

      srf.Data.B = buffer_;
      srf.Data.G = srf.Data.B + 1;
      srf.Data.R = srf.Data.B + 2;
      srf.Data.A = srf.Data.B + 3;
      break;
    default:
      g_assert_not_reached();
      return MFX_ERR_UNSUPPORTED;
    }
    return MFX_ERR_NONE;
  }

private:
  mfxU8 * buffer_;

private:
  MfxFrameSurface1Wrap(const MfxFrameSurface1Wrap&);
  MfxFrameSurface1Wrap& operator=(const MfxFrameSurface1Wrap&);
};

template<typename T, typename... ExtBufs>
struct MfxExtBufHolder: public T
{
  static const int N = sizeof...(ExtBufs);
  static_assert(N, "mfxExtBufHolder<T, ExtBufs...>: ExtBufs is required");

  mfxExtBuffer* ext_buf_ptrs[N];
  static const int max_sizeof = mfx_utils::MaxSizeof<ExtBufs...>::max_sizeof;
  union {
    mfxExtBuffer buf;
    mfxU8 buf2[max_sizeof];
  } ext_buf[N];
  struct
  {
      bool enabled;
      int idx;
  } ext_buf_idxmap[N];

  MfxExtBufHolder()
  {
    memset(static_cast<T*>(this), 0, sizeof(T));

    MSDK_ZERO_MEM(ext_buf_ptrs);
    MSDK_ZERO_MEM(ext_buf);
    MSDK_ZERO_MEM(ext_buf_idxmap);

    this->ExtParam = ext_buf_ptrs;
    for (size_t i = 0; i < N; ++i) {
      ext_buf_ptrs[i] = &ext_buf[i].buf;
    }
  }
  MfxExtBufHolder(const MfxExtBufHolder& ref)
  {
    *this = ref; // call to operator=
  }
  MfxExtBufHolder& operator=(const MfxExtBufHolder& ref)
  {
    T* dst = this;
    const T* src = &ref;

    memcpy(dst, src, sizeof(T));

    MSDK_ZERO_MEM(ext_buf_ptrs);
    memcpy(ext_buf, ref.ext_buf, sizeof(ref.ext_buf));
    memcpy(ext_buf_idxmap, ref.ext_buf_idxmap, sizeof(ref.ext_buf_idxmap));

    this->ExtParam = ext_buf_ptrs;
    for (size_t i = 0; i < N; ++i) {
        ext_buf_ptrs[i] = &ext_buf[i].buf;
    }
    return *this;
  }
  MfxExtBufHolder(const T& ref)
  {
    *this = ref; // call to operator=
  }
  MfxExtBufHolder& operator=(const T& ref)
  {
    T* dst = this;
    const T* src = &ref;

    memcpy(dst, src, sizeof(T));

    MSDK_ZERO_MEM(ext_buf_ptrs);
    MSDK_ZERO_MEM(ext_buf);
    MSDK_ZERO_MEM(ext_buf_idxmap);

    this->ExtParam = ext_buf_ptrs;
    for (size_t i = 0; i < N; ++i) {
      ext_buf_ptrs[i] = &ext_buf[i].buf;
    }
    return *this;
  }
  void ResetExtParams()
  {
    this->NumExtParam = 0;
    this->ExtParam = ext_buf_ptrs;
  }

  // Function returns pointer to the already enabled buffer.
  template<typename ExtBufType>
  ExtBufType* get() const
  {
    int idx_map = getEnabledMapIdx(mfx_utils::mfx_ext_buffer_id<ExtBufType>::id);

    if (idx_map < 0) {
      return NULL;
    }
    if (ext_buf_idxmap[idx_map].enabled) {
      return (ExtBufType*)&this->ext_buf[ext_buf_idxmap[idx_map].idx];
    }
    return NULL;
  }

  // Function enables extended buffer and returns ponter to it.
  template<typename ExtBufType>
  ExtBufType* enable()
  {
    mfxU32 bufferid = mfx_utils::mfx_ext_buffer_id<ExtBufType>::id;
    int idx_map = getEnabledMapIdx(bufferid);

    if (idx_map < 0) return NULL;

    if (this->ext_buf_idxmap[idx_map].enabled) {
      return (ExtBufType*)&this->ext_buf[this->ext_buf_idxmap[idx_map].idx];
    }

    if (this->NumExtParam >= N) {
      return NULL;
    }

    int idx = this->NumExtParam++;

    this->ext_buf_idxmap[idx_map].enabled = true;
    this->ext_buf_idxmap[idx_map].idx = idx;

    MSDK_ZERO_MEM(this->ext_buf[idx]);
    this->ext_buf[idx].buf.BufferId = bufferid;
    this->ext_buf[idx].buf.BufferSz = sizeof(T);
    return (ExtBufType*)&this->ext_buf[idx];
  }

protected:
  int getEnabledMapIdx(mfxU32 bufferid) const
  {
    mfxU32 ids[N] = { mfx_utils::mfx_ext_buffer_id<ExtBufs>::id... };
    for (int idx = 0; idx < N; ++idx) {
      if (bufferid == ids[idx]) return idx;
    }
    return -1;
  }
};

template<typename T>
struct MfxExtBufHolder<T> : public T
{
  MfxExtBufHolder()
  {
    memset(static_cast<T*>(this), 0, sizeof(T));
  }
};

typedef MfxExtBufHolder<mfxVideoParam> MfxVideoParamWrap;
typedef MfxExtBufHolder<mfxEncodeCtrl> MfxEncodeCtrlWrap;

typedef MfxExtBufHolder<
    mfxVideoParam
  // list of extended buffer types we want to handle
  , mfxExtVPPDeinterlacing
  , mfxExtVPPDenoise
  > MfxVideoVppParamWrap;

#endif
