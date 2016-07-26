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

#ifndef __MFX_GST_BITSTREAM_BUFFER_H__
#define __MFX_GST_BITSTREAM_BUFFER_H__

#include <memory>

#include "mfx_wrappers.h"
#include "mfx_gst_debug.h"
#include "mfx_gst_buffer.h"
#include "mfx_gst_buffer_pool.h"

struct mfxGstBitstreamBufferRef: public mfxGstBufferRef
{
  mfxGstBitstreamBufferRef(GstBuffer* buffer)
    : mfxGstBufferRef(buffer)
  {
  }

  // Returns mediasdk bitstream representation wrapped by this object.
  // Function may return NULL if map() operation failed.
  inline MfxBistreamWrap* bst() {
    if (!mem_ && !map(GST_MAP_WRITE)) return NULL;
    return &bst_;
  }

  inline mfxSyncPoint* syncp() {
    return &syncp_;
  }

protected:
  virtual bool map(GstMapFlags purpose);
  virtual void unmap();

protected:
  MfxBistreamWrap bst_;
  std::shared_ptr<MfxEncodeCtrlWrap> ctrl_;
  mfxSyncPoint syncp_;
};

class mfxGstBufferPoolWrap
{
public:
  mfxGstBufferPoolWrap()
    : pool_(NULL)
  {}
  ~mfxGstBufferPoolWrap() { Close(); }

  bool Init(guint num_buffers, guint size);
  void Close();

  std::shared_ptr<mfxGstBitstreamBufferRef> GetBuffer();

protected:
  GstBufferPool *pool_;

private:
  mfxGstBufferPoolWrap(const mfxGstBufferPoolWrap&);
  mfxGstBufferPoolWrap& operator=(const mfxGstBufferPoolWrap&);
};

#endif
