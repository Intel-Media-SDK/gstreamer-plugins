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
#define MFX_DEBUG_MODULE_NAME "mfxgst_bstbuf"

bool mfxGstBitstreamBufferRef::map(GstMapFlags purpose)
{
  MFX_DEBUG_TRACE_FUNC;
  bool res = mfxGstBufferRef::map(purpose);
  if (res) {
    bst_.Data = meminfo_.data;
    bst_.MaxLength = meminfo_.maxsize;
    bst_.DataLength = meminfo_.size;
    bst_.DataOffset = 0;
    bst_.TimeStamp = GST_BUFFER_TIMESTAMP(buffer_);
    bst_.DecodeTimeStamp = GST_BUFFER_DTS(buffer_);
  }
  return res;
}

void mfxGstBitstreamBufferRef::unmap()
{
  MFX_DEBUG_TRACE_FUNC;
  if (mem_) {
    gst_memory_resize(mem_, mem_->offset, bst_.DataOffset + bst_.DataLength);

    GST_BUFFER_FLAG_SET(buffer_, 0);
    GST_BUFFER_TIMESTAMP(buffer_) = bst_.TimeStamp;
    GST_BUFFER_DTS(buffer_) = bst_.DecodeTimeStamp;

    if (!(bst_.FrameType & MFX_FRAMETYPE_IDR)) {
      GST_BUFFER_FLAG_SET(buffer_, GST_BUFFER_FLAG_DELTA_UNIT);
    }
  }
  mfxGstBufferRef::unmap();
}

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstbufferpool"

bool mfxGstBufferPoolWrap::Init(guint num_buffers, guint size)
{
  MFX_DEBUG_TRACE_FUNC;
  MFX_DEBUG_TRACE_I32(num_buffers);
  MFX_DEBUG_TRACE_I32(size);
  GstStructure* config = NULL;
  GstCaps *caps = NULL;
  gboolean res;

  if (pool_) {
    MFX_DEBUG_TRACE_MSG("pool is already inititialized");
    return false;
  }

  pool_ = gst_mfx_buffer_pool_new();
  if (!pool_) {
    MFX_DEBUG_TRACE_MSG("failed to allocate the pool");
    goto error;
  }

  config = gst_buffer_pool_get_config(pool_);
  if (!config) {
    MFX_DEBUG_TRACE_MSG("failed to get config structure");
    goto error;
  }

  caps = gst_caps_new_empty_simple ("test/data"); // FIXME
  gst_buffer_pool_config_set_params(config, caps, size, num_buffers, 0);

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

void mfxGstBufferPoolWrap::Close()
{
  if (pool_) {
    gst_buffer_pool_set_active(pool_, FALSE);
    gst_object_unref(pool_);
    pool_ = NULL;
  }
}

std::shared_ptr<mfxGstBitstreamBufferRef> mfxGstBufferPoolWrap::GetBuffer()
{
  MFX_DEBUG_TRACE_FUNC;
  GstBuffer* buffer = NULL;

  if (GST_FLOW_OK != gst_buffer_pool_acquire_buffer(pool_, &buffer, NULL)) { // This is a blocking function !
    return NULL;
  }
  MFX_DEBUG_TRACE_P(buffer);
  if (!buffer) {
    MFX_DEBUG_TRACE_MSG("failed to acquire buffer from pool");
    return NULL;
  }

  gst_buffer_set_size(buffer, 0);

  std::shared_ptr<mfxGstBitstreamBufferRef> buffer_ref = std::make_shared<mfxGstBitstreamBufferRef>(buffer);
  gst_buffer_unref(buffer);
  return buffer_ref;
}
