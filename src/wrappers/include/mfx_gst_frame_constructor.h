/**********************************************************************************

Copyright (C) 2005-2019 Intel Corporation.  All rights reserved.

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

#ifndef __MFX_GST_FRAME_CONSTRUCTOR_H__
#define __MFX_GST_FRAME_CONSTRUCTOR_H__

#include "mfx_gst_bitstream_buffer.h"

enum MfxGstBitstreamState
{
  MfxGstBS_HeaderAwaiting = 0,
  MfxGstBS_HeaderCollecting = 1,
  MfxGstBS_HeaderObtained = 2,
  MfxGstBS_Resetting = 3,
};

enum MfxGstFrameConstuctorType
{
  MfxGstFC_NoChange,
  MfxGstFC_AVCC
};

class IMfxGstFrameConstuctor
{
public:
  IMfxGstFrameConstuctor();
  virtual ~IMfxGstFrameConstuctor(void);

  virtual mfxStatus Init() { return MFX_ERR_NONE; }

  virtual mfxStatus Load(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref, bool header);

  virtual mfxStatus Unload(void);

  virtual mfxStatus Reset(void);

  virtual void Close(void) { Reset(); }

  virtual mfxBitstream * GetMfxBitstream(void);

protected:
  virtual mfxStatus LoadToNativeFormat(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref, bool header) = 0;

  virtual mfxStatus Sync(void);

protected:

  struct MfxBistreamBuffer: public mfxBitstream
  {
    MfxBistreamBuffer()
    {
      memset((mfxBitstream*)this, 0, sizeof(mfxBitstream));
    }
    mfxStatus Append(const mfxBitstream * const bst)
    {
      return bst ? this->Append(*bst) : MFX_ERR_NULL_PTR;
    }
    mfxStatus Append(const mfxBitstream & bst)
    {
      if (!this->Data) // no memory
      {
        this->Data = (mfxU8*)malloc(bst.DataLength);
        if (!this->Data) return MFX_ERR_MEMORY_ALLOC;

        memcpy(this->Data, bst.Data + bst.DataOffset, bst.DataLength);
        this->DataOffset = 0;
        this->DataLength = bst.DataLength;
        this->MaxLength = bst.DataLength;
        this->TimeStamp = bst.TimeStamp;
        this->DataFlag = bst.DataFlag;
      }
      else if (!this->DataLength && (this->MaxLength < bst.DataLength)) // no buffered data and some memory(not enough)
      {
        free(this->Data);
        this->Data = (mfxU8*)malloc(bst.DataLength);
        if (!this->Data) return MFX_ERR_MEMORY_ALLOC;

        memcpy(this->Data, bst.Data + bst.DataOffset, bst.DataLength);
        this->DataOffset = 0;
        this->DataLength = bst.DataLength;
        this->MaxLength = bst.DataLength;
        this->TimeStamp = bst.TimeStamp;
        this->DataFlag = bst.DataFlag;
      }
      else if (!this->DataLength && (this->MaxLength >= bst.DataLength)) // no buffered data and enough memory
      {
        memcpy(this->Data, bst.Data + bst.DataOffset, bst.DataLength);
        this->DataOffset = 0;
        this->DataLength = bst.DataLength;
        this->TimeStamp = bst.TimeStamp;
        this->DataFlag = bst.DataFlag;
      }
      else if (this->DataLength && ((this->MaxLength - this->DataOffset - this->DataLength) < bst.DataLength)) // buffered data and some memory(not enough)
      {
        this->Data = (mfxU8*)realloc(this->Data, this->DataOffset + this->DataLength + bst.DataLength);
        if (!this->Data) return MFX_ERR_MEMORY_ALLOC;

        memcpy(this->Data + this->DataOffset + this->DataLength, bst.Data + bst.DataOffset, bst.DataLength);
        this->DataLength += bst.DataLength;
        this->MaxLength = this->DataOffset + this->DataLength + bst.DataLength;
        this->TimeStamp = bst.TimeStamp;
      }
      else if (this->DataLength && ((this->MaxLength - this->DataOffset - this->DataLength) >= bst.DataLength)) // buffered data and enough memory
      {
        memcpy(this->Data + this->DataOffset + this->DataLength, bst.Data + bst.DataOffset, bst.DataLength);
        this->DataLength += bst.DataLength;
        this->TimeStamp = bst.TimeStamp;
      }

      return MFX_ERR_NONE;
    }
    void Reset()
    {
      mfxU8 * data = this->Data;
      mfxU32 length = this->MaxLength;

      memset((mfxBitstream*)this, 0, sizeof(mfxBitstream));

      this->Data = data;
      this->MaxLength = length;
    }
    mfxStatus Extend(mfxU32 add)
    {
      this->Data = (mfxU8*)realloc(this->Data, this->MaxLength + add);
      if (!this->Data) return MFX_ERR_MEMORY_ALLOC;

      this->MaxLength = this->MaxLength + add;
      return MFX_ERR_NONE;
    }
  };

  MfxGstBitstreamState m_bs_state;
  MfxBistreamBuffer buffered_bst_;
  std::shared_ptr<mfxGstBitstreamBufferRef> current_bst_;

private:
  IMfxGstFrameConstuctor(const IMfxGstFrameConstuctor&);
  IMfxGstFrameConstuctor& operator=(const IMfxGstFrameConstuctor&);
};


class MfxGstFrameConstuctor : public IMfxGstFrameConstuctor
{
public:
  MfxGstFrameConstuctor() = default;
  MfxGstFrameConstuctor(const MfxGstFrameConstuctor&) = delete;
  MfxGstFrameConstuctor& operator=(const MfxGstFrameConstuctor&) = delete;

  virtual ~MfxGstFrameConstuctor(void) = default;

protected:
  virtual mfxStatus LoadToNativeFormat(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref, bool header);
};

class MfxGstAVCFrameConstuctorAVCC : public IMfxGstFrameConstuctor
{
public:
  MfxGstAVCFrameConstuctorAVCC() = default;
  MfxGstAVCFrameConstuctorAVCC(const MfxGstAVCFrameConstuctorAVCC&) = delete;
  MfxGstAVCFrameConstuctorAVCC& operator=(const MfxGstAVCFrameConstuctorAVCC&) = delete;
  virtual ~MfxGstAVCFrameConstuctorAVCC(void) = default;

protected:
  virtual mfxStatus LoadToNativeFormat(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref, bool header);
  mfxStatus ConstructHeader(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref);
};

class MfxBitstreamLoader
{
public:
  MfxBitstreamLoader(IMfxGstFrameConstuctor * fc) :
      fc_(fc),
      loaded_(false)
  {
      MFX_DEBUG_TRACE_FUNC;
  }
  virtual ~MfxBitstreamLoader(void)
  {
      MFX_DEBUG_TRACE_FUNC;
      if (loaded_)
      {
          fc_->Unload();
          loaded_ = false;
      }
  }

  mfxStatus LoadBuffer(std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref_, bool header)
  {
      MFX_DEBUG_TRACE_FUNC;
      mfxStatus sts = MFX_ERR_NONE;

      if (loaded_)
      {
          sts = fc_->Unload();
          loaded_ = false;
      }
      if (MFX_ERR_NONE == sts)
      {
          sts = fc_->Load(bst_ref_, header);
          loaded_ = true;
      }

      MFX_DEBUG_TRACE_I32(sts);
      return sts;
  }

protected:
  IMfxGstFrameConstuctor * fc_;
  bool loaded_;

private:
  MfxBitstreamLoader(const MfxBitstreamLoader&);
  MfxBitstreamLoader& operator=(const MfxBitstreamLoader&);
};

class MfxGstFrameConstuctorFactory
{
public:
    static IMfxGstFrameConstuctor* CreateFrameConstuctor(MfxGstFrameConstuctorType type);
};

#endif
