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

#include <exception>
#include <cassert>
#include <stdlib.h>
#include "mfx_utils.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfx_utils"

void mfx_gst_copy_srf(const mfxFrameSurface1 & dst, const mfxFrameSurface1 & src)
{
  MFX_DEBUG_TRACE_FUNC;

  if (src.Info.FourCC != dst.Info.FourCC) {
    g_assert_not_reached();
    return;
  }
  switch (src.Info.FourCC)
  {
  case MFX_FOURCC_NV12:
    for (mfxU32 i = 0; i < src.Info.CropH/2; ++i) {
      // copying Y
      memcpy(dst.Data.Y + i * dst.Data.Pitch,
              src.Data.Y + i * src.Data.Pitch,
              src.Info.Width);
      // copying UV
      memcpy(dst.Data.UV + i * dst.Data.Pitch,
              src.Data.UV + i * src.Data.Pitch,
              src.Info.Width);
    }
    for (mfxU32 i = src.Info.CropH/2; i < src.Info.CropH; ++i) {
      // copying Y (remained data)
      memcpy(dst.Data.Y + i * dst.Data.Pitch,
              src.Data.Y + i * src.Data.Pitch,
              src.Info.Width);
    }
    break;
  case MFX_FOURCC_YV12:
    for (mfxU32 i = 0; i < src.Info.CropH; ++i) {
      memcpy(dst.Data.Y + i * dst.Data.Pitch,
              src.Data.Y + i * MSDK_ALIGN4(src.Info.Width),
              src.Info.Width);
    }
    for (mfxU32 i = 0; i < src.Info.CropH/2; ++i) {
      memcpy(dst.Data.V + i * dst.Data.Pitch/2,
              src.Data.V + i * MSDK_ALIGN4(MSDK_ALIGN2(src.Info.Width)/2),
              src.Info.Width/2);
    }
    for (mfxU32 i = 0; i < src.Info.CropH/2; ++i) {
      memcpy(dst.Data.U + i * dst.Data.Pitch/2,
              src.Data.U + i * MSDK_ALIGN4(MSDK_ALIGN2(src.Info.Width)/2),
              src.Info.Width/2);
    }
    break;
  case MFX_FOURCC_YUY2:
    for(mfxU32 i = 0; i < src.Info.CropH; i++) {
      memcpy(dst.Data.Y + i * dst.Data.Pitch,
             src.Data.Y + i * src.Data.Pitch,
             2 * src.Info.Width);
    }
    break;
   case MFX_FOURCC_UYVY:
     for(mfxU32 i = 0; i < src.Info.CropH; i++) {
       memcpy(dst.Data.U + i * dst.Data.Pitch,
              src.Data.U + i * src.Data.Pitch,
              2 * src.Info.Width);
     }
     break;
  case MFX_FOURCC_RGB4:
    for(mfxU32 i = 0; i < src.Info.CropH; i++) {
      memcpy(dst.Data.B + i * dst.Data.Pitch,
             src.Data.B + i * src.Data.Pitch,
             4 * src.Info.Width);
    }
    break;
  default:
    g_assert_not_reached();
    return;
  }
}

class EndOfBuffer : public std::exception
{
public:
  EndOfBuffer() : std::exception() {}
};

BitstreamWriter::BitstreamWriter(mfxU8 * buf, size_t size, bool emulationControl)
: m_buf(buf)
, m_ptr(buf)
, m_bufEnd(buf + size)
, m_bitOff(0)
, m_emulationControl(emulationControl)
{
    if (m_ptr < m_bufEnd)
        *m_ptr = 0; // clear next byte
}

BitstreamWriter::BitstreamWriter(mfxU8 * buf, mfxU8 * bufEnd, bool emulationControl)
: m_buf(buf)
, m_ptr(buf)
, m_bufEnd(bufEnd)
, m_bitOff(0)
, m_emulationControl(emulationControl)
{
    if (m_ptr < m_bufEnd)
        *m_ptr = 0; // clear next byte
}

mfxU32 BitstreamWriter::GetNumBits() const
{
    return mfxU32(8 * (m_ptr - m_buf) + m_bitOff);
}

void BitstreamWriter::PutBit(mfxU32 bit)
{
    if (m_ptr >= m_bufEnd)
        throw EndOfBuffer();

    mfxU8 mask = mfxU8(0xff << (8 - m_bitOff));
    mfxU8 newBit = mfxU8((bit & 1) << (7 - m_bitOff));
    *m_ptr = (*m_ptr & mask) | newBit;

    if (++m_bitOff == 8)
    {
        if (m_emulationControl && m_ptr - 2 >= m_buf &&
            (*m_ptr & 0xfc) == 0 && *(m_ptr - 1) == 0 && *(m_ptr - 2) == 0)
        {
            if (m_ptr + 1 >= m_bufEnd)
                throw EndOfBuffer();

            *(m_ptr + 1) = *(m_ptr + 0);
            *(m_ptr + 0) = 0x03;
            m_ptr++;
        }

        m_bitOff = 0;
        m_ptr++;
        if (m_ptr < m_bufEnd)
            *m_ptr = 0; // clear next byte
    }
}

void BitstreamWriter::PutBits(mfxU32 val, mfxU32 nbits)
{
    assert(nbits <= 32);

    for (; nbits > 0; nbits--)
        PutBit((val >> (nbits - 1)) & 1);
}

void BitstreamWriter::PutRawBytes(mfxU8 const * begin, mfxU8 const * end)
{
    assert(m_bitOff == 0);

    if (m_bufEnd - m_ptr < end - begin)
        throw EndOfBuffer();

    memcpy(m_ptr, begin, (mfxU32)(end - begin));
    m_bitOff = 0;
    m_ptr += end - begin;

    if (m_ptr < m_bufEnd)
        *m_ptr = 0;
}

NalUnit GetNalUnit(mfxU8 * begin, mfxU8 * end)
{
  for (; begin < end - 5; ++begin) {
    if ((begin[0] == 0 && begin[1] == 0 && begin[2] == 1) ||
        (begin[0] == 0 && begin[1] == 0 && begin[2] == 0 && begin[3] == 1)) {
      mfxU8 numZero = (begin[2] == 1 ? 2 : 3);
      mfxU8 type    = (begin[2] == 1 ? begin[3] : begin[4]) & 0x1f;

      for (mfxU8 * next = begin + 4; next < end - 4; ++next) {
        if (next[0] == 0 && next[1] == 0 && next[2] == 1) {
          if (*(next - 1) == 0) {
              --next;
          }

          return NalUnit(begin, next, type, numZero);
        }
      }

      return NalUnit(begin, end, type, numZero);
    }
  }

  return NalUnit();
}

bool operator ==(NalUnit const & left, NalUnit const & right)
{
    return left.begin == right.begin && left.end == right.end;
}

bool operator !=(NalUnit const & left, NalUnit const & right)
{
    return !(left == right);
}

static inline void
set_u32(mfxU8 * ptr, mfxU32 value)
{
  if (ptr) {
    ptr[0] = ((value >> 24) & 0xff);
    ptr[1] = ((value >> 16) & 0xff);
    ptr[2] = ((value >> 8) & 0xff);
    ptr[3] = (value & 0xff);
  }
}

static inline mfxU32
get_u32(mfxU8 * ptr)
{
  return ptr ? ((*ptr) << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | (*(ptr + 3)) : 0;
}

mfxStatus
write_nalu_length(mfxBitstream* bst)
{
  MFX_DEBUG_TRACE_FUNC;
  guint32 nal_size = 0;
  guint8 *begin = NULL;
  guint8 *end= NULL;

  if (!bst)
    return MFX_ERR_NULL_PTR;

  begin = bst->Data + bst->DataOffset;
  end = bst->Data + bst->DataOffset + bst->DataLength;

  for (NalUnit nalu = GetNalUnit(begin, end); nalu != NalUnit(); nalu = GetNalUnit(begin, end))
  {
    begin = nalu.begin;
    nal_size = nalu.end - nalu.begin - nalu.numZero - 1;

    if (nalu.numZero == 2) {
      // we have to make sure we have enought bitstream length
      if (bst->MaxLength >= (bst->DataOffset + bst->DataLength + 1)) {
        // we're waiting for a new API in MediaSDK
        // which will allow to get rid of 3 byte start code.
        // Once API is ready, the code below will be removed
        memmove(begin + 1, begin, end - begin);
        nalu.end++;
        end++;
        bst->DataLength++;
      }
      else {
        MFX_DEBUG_TRACE_MSG("MFX_ERR_NOT_ENOUGH_BUFFER");
        return MFX_ERR_NOT_ENOUGH_BUFFER;
      }
    }

    set_u32(begin, nal_size);
    begin = nalu.end;
  }

  return MFX_ERR_NONE;
}

mfxStatus
convert_avcc_to_bytestream(mfxBitstream * bst)
{
  MFX_DEBUG_TRACE_FUNC;
  guint32 nal_size = 0;
  guint8 * begin = NULL;
  guint8 * end = NULL;

  if (!bst) {
    return MFX_ERR_NULL_PTR;
  }

  begin = bst->Data + bst->DataOffset;
  end = bst->Data + bst->DataOffset + bst->DataLength;

  for (; begin < end - 5;)
  {
    nal_size = get_u32(begin);
    set_u32(begin, 0x00000001);
    begin += 4 + nal_size;
  }

  return MFX_ERR_NONE;
}

bool IsDIEnabled(const mfxVideoParam & par)
{
  if ((par.vpp.In.PicStruct & (MFX_PICSTRUCT_FIELD_BFF | MFX_PICSTRUCT_FIELD_TFF)) &&
    (MFX_PICSTRUCT_PROGRESSIVE == par.vpp.Out.PicStruct)) {
    return true;
  }
  else if ((MFX_PICSTRUCT_UNKNOWN == par.vpp.In.PicStruct) &&
    (MFX_PICSTRUCT_PROGRESSIVE == par.vpp.Out.PicStruct)) {
    return true;
  }

  return false;
}
