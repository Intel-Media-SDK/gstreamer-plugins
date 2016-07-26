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
#include "mfx_gst_frame_constructor.h"
#include "mfx_utils.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfx_gst_frame_constructor"

/*------------------------------------------------------------------------------*/

IMfxGstFrameConstuctor::IMfxGstFrameConstuctor():
  m_bs_state(MfxGstBS_HeaderAwaiting)
{
  MFX_DEBUG_TRACE_FUNC;
}

/*------------------------------------------------------------------------------*/

IMfxGstFrameConstuctor::~IMfxGstFrameConstuctor(void)
{
  MFX_DEBUG_TRACE_FUNC;
  if (buffered_bst_.Data) {
    MSDK_FREE(buffered_bst_.Data);
  }
}

/*------------------------------------------------------------------------------*/

mfxStatus IMfxGstFrameConstuctor::Load(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref, bool header)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxStatus sts = LoadToNativeFormat(bst_ref, header);

  if (MFX_ERR_NONE != sts) {
    Unload();
  }

  MFX_DEBUG_TRACE_I32(sts);
  return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus IMfxGstFrameConstuctor::Unload(void)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxStatus sts = Sync();
  current_bst_.reset();

  MFX_DEBUG_TRACE_I32(sts);
  return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus IMfxGstFrameConstuctor::Sync(void)
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts = MFX_ERR_NONE;

  if (buffered_bst_.DataLength && buffered_bst_.DataOffset) {
      memmove(buffered_bst_.Data, buffered_bst_.Data + buffered_bst_.DataOffset, buffered_bst_.DataLength);
  }
  buffered_bst_.DataOffset = 0;

  mfxBitstream * bst = current_bst_.get() ? current_bst_->bst() : NULL;
  if (bst && bst->DataLength) {
    sts = buffered_bst_.Append(bst);
  }

  MFX_DEBUG_TRACE_I32(sts);
  return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus IMfxGstFrameConstuctor::Reset(void)
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts = MFX_ERR_NONE;

  buffered_bst_.Reset();

  if (m_bs_state >= MfxGstBS_HeaderCollecting) m_bs_state = MfxGstBS_Resetting;

  MFX_DEBUG_TRACE_I32(sts);
  return sts;
}

/*------------------------------------------------------------------------------*/

mfxBitstream * IMfxGstFrameConstuctor::GetMfxBitstream(void)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxBitstream * bst = NULL;

  mfxBitstream * loaded_bst = current_bst_.get() ? current_bst_->bst() : NULL;

  if (buffered_bst_.Data && buffered_bst_.DataLength) {
      bst = &buffered_bst_;
  }
  else if (loaded_bst && loaded_bst->Data && loaded_bst->DataLength) {
      bst = loaded_bst;
  }
  else {
    bst = &buffered_bst_;
  }

  MFX_DEBUG_TRACE_P(&buffered_bst_);
  MFX_DEBUG_TRACE_P(&loaded_bst);
  MFX_DEBUG_TRACE_P(bst);
  return bst;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxGstAVCFrameConstuctor::LoadToNativeFormat(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref, bool header)
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts = MFX_ERR_NONE;

  if (header) {
    ConstructHeader(bst_ref);
  }
  else {
    mfxBitstream * bst = bst_ref.get() ? bst_ref->bst() : NULL;

    convert_avcc_to_bytestream(bst);

    if (buffered_bst_.DataLength) {
      sts = buffered_bst_.Append(bst);
      current_bst_.reset();
    }
    else {
      current_bst_ = bst_ref;
    }
  }

  MFX_DEBUG_TRACE_I32(sts);
  return sts;
}

mfxStatus MfxGstAVCFrameConstuctor::ConstructHeader(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref)
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts = MFX_ERR_NONE;
  mfxU8 * data = NULL;
  mfxU32 size = 0;
  mfxU32 numOfSequenceParameterSets = 0;
  mfxU32 numOfPictureParameterSets = 0;
  mfxU32 sequenceParameterSetLength = 0;
  mfxU32 pictureParameterSetLength = 0;
  static mfxU8 header[4] = {0x00, 0x00, 0x00, 0x01};
  MfxBistreamBuffer bst_header;

  mfxBitstream * in_bst = bst_ref.get() ? bst_ref->bst() : NULL;
  if (!in_bst) {
    sts = MFX_ERR_NULL_PTR;
    goto _done;
  }

  if ((bst_header.Extend(1024) != MFX_ERR_NONE) || (!bst_header.Data)) {
    sts = MFX_ERR_MEMORY_ALLOC;
    goto _done;
  }

  data = in_bst->Data;
  size = in_bst->DataLength;

  /* AVCDecoderConfigurationRecord:
    *  - mfxU8 - configurationVersion = 1
    *  - mfxU8 - AVCProfileIndication
    *  - mfxU8 - profile_compatibility
    *  - mfxU8 - AVCLevelIndication
    *  - mfxU8 - '111111'b + lengthSizeMinusOne
    *  - mfxU8 - '111'b + numOfSequenceParameterSets
    *  - for (i = 0; i < numOfSequenceParameterSets; ++i) {
    *      - mfxU16 - sequenceParameterSetLength
    *      - mfxU8*sequenceParameterSetLength - SPS
    *    }
    *  - mfxU8 - numOfPictureParameterSets
    *  - for (i = 0; i < numOfPictureParameterSets; ++i) {
    *      - mfxU16 - pictureParameterSetLength
    *      - mfxU8*pictureParameterSetLength - PPS
    *    }
    */
  if (size < 5) {
    sts = MFX_ERR_NOT_ENOUGH_BUFFER;
    goto _done;
  }
  data += 5;
  size -= 5;

  if (size < 1) {
    sts = MFX_ERR_NOT_ENOUGH_BUFFER;
    goto _done;
  }
  numOfSequenceParameterSets = data[0] & 0x1f;
  ++data;
  --size;

  for (mfxU32 i = 0; i < numOfSequenceParameterSets; ++i) {
    if (size < 2) {
      sts = MFX_ERR_NOT_ENOUGH_BUFFER;
      goto _done;
    }
    sequenceParameterSetLength = (data[0] << 8) | data[1];
    data += 2;
    size -= 2;

    if (size < sequenceParameterSetLength) {
      sts = MFX_ERR_NOT_ENOUGH_BUFFER;
      goto _done;
    }

    if (bst_header.MaxLength - bst_header.DataOffset + bst_header.DataLength < 4 + sequenceParameterSetLength) {
      if (MFX_ERR_NONE != bst_header.Extend(4 + sequenceParameterSetLength)) {
        sts = MFX_ERR_NOT_ENOUGH_BUFFER;
        goto _done;
      }
    }

    mfxU8 * buf = bst_header.Data + bst_header.DataOffset + bst_header.DataLength;
    memcpy(buf, header, sizeof(mfxU8)*4);
    memcpy(&(buf[4]), data, sizeof(mfxU8)*sequenceParameterSetLength);
    data += sequenceParameterSetLength;
    size -= sequenceParameterSetLength;

    bst_header.DataLength += 4 + sequenceParameterSetLength;
  }

  if (size < 1) {
    sts = MFX_ERR_NOT_ENOUGH_BUFFER;
    goto _done;
  }
  numOfPictureParameterSets = data[0];
  ++data;
  --size;

  for (mfxU32 i = 0; i < numOfPictureParameterSets; ++i) {
    if (size < 2) {
      sts = MFX_ERR_NOT_ENOUGH_BUFFER;
      goto _done;
    }
    pictureParameterSetLength = (data[0] << 8) | data[1];
    data += 2;
    size -= 2;

    if (size < pictureParameterSetLength) {
      sts = MFX_ERR_NOT_ENOUGH_BUFFER;
      goto _done;
    }

    if (bst_header.MaxLength - bst_header.DataOffset + bst_header.DataLength < 4 + pictureParameterSetLength) {
      if (MFX_ERR_NONE != bst_header.Extend(4 + pictureParameterSetLength)) {
        sts = MFX_ERR_NOT_ENOUGH_BUFFER;
        goto _done;
      }
    }

    mfxU8 * buf = bst_header.Data + bst_header.DataOffset + bst_header.DataLength;
    memcpy(buf, header, sizeof(mfxU8)*4);
    memcpy(&(buf[4]), data, sizeof(mfxU8)*pictureParameterSetLength);
    data += pictureParameterSetLength;
    size -= pictureParameterSetLength;

    bst_header.DataLength += 4 + pictureParameterSetLength;
  }

  sts = buffered_bst_.Append(bst_header);

  _done:
  return sts;
}


IMfxGstFrameConstuctor* MfxGstFrameConstuctorFactory::CreateFrameConstuctor(MfxGstFrameConstuctorType type)
{
    IMfxGstFrameConstuctor* fc = NULL;
    if (MfxGstFC_AVC == type)
    {
        fc = new MfxGstAVCFrameConstuctor;
        return fc;
    }
    return NULL;
}

