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

#ifndef __MFX_GST_BUFFER_H__
#define __MFX_GST_BUFFER_H__

#include <memory>

#include "mfx_gst_debug.h"

struct ImfxGstBuffer {
  virtual ~ImfxGstBuffer() {}
  virtual GstBuffer* getBuffer() = 0;
};

struct mfxGstBufferRef : public ImfxGstBuffer {
  mfxGstBufferRef(GstBuffer* buffer)
    : buffer_(buffer)
    , mem_(NULL)
  {
    gst_buffer_ref(buffer);
    MSDK_ZERO_MEM(meminfo_);
  }
  virtual ~mfxGstBufferRef()
  {
    unmap();
    gst_buffer_unref(buffer_);
  }

  // Returns gstreamer buffer representation wrapped by this object.
  // Caller is responsible to release the buffer.
  virtual GstBuffer* getBuffer() {
    if (buffer_) {
      unmap();
      gst_buffer_ref(buffer_);
    }
    return buffer_;
  }

protected:
  virtual bool map(GstMapFlags purpose);
  virtual void unmap();

protected:
  GstBuffer* buffer_;
  GstMemory* mem_;
  GstMapInfo meminfo_;
};

#endif
