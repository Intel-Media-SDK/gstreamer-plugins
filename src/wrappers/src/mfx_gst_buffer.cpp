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

#include "mfx_gst_bitstream_buffer.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstbuffer"

bool mfxGstBufferRef::map(GstMapFlags purpose)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_P(buffer_);
  if (mem_) {
    MFX_DEBUG_TRACE_MSG("I am not gonna to map it twice!");
    return false;
  }
  MFX_DEBUG_TRACE_I32(gst_buffer_n_memory(buffer_));
  mem_ = gst_buffer_peek_memory(buffer_, 0);
  if (!mem_) {
    MFX_DEBUG_TRACE_MSG("failed: gst_buffer_peek_memory");
    return false;
  }
  if (!gst_memory_map(mem_, &meminfo_, purpose)) {
    MFX_DEBUG_TRACE_MSG("failed: gst_memory_map");
    return false;
  }
  MFX_DEBUG_TRACE_I32(mem_->align);
  MFX_DEBUG_TRACE_I32(mem_->offset);
  MFX_DEBUG_TRACE_I32(mem_->maxsize);
  MFX_DEBUG_TRACE_I32(mem_->size);

  MFX_DEBUG_TRACE_P(meminfo_.data);
  MFX_DEBUG_TRACE_I32(meminfo_.maxsize);
  MFX_DEBUG_TRACE_I32(meminfo_.size);
  return true;
}

void mfxGstBufferRef::unmap()
{
  MFX_DEBUG_TRACE_FUNC;
  if (mem_) {
    gst_memory_unmap(mem_, &meminfo_);
    mem_ = NULL;
    memset(&meminfo_, 0, sizeof(meminfo_));
  }
}
