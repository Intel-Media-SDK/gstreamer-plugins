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

#ifndef __MFX_UTILS_H__
#define __MFX_UTILS_H__

#include <string.h>
#include <vector>

#include "mfx_defs.h"
#include "mfx_gst_props.h"
#include "mfxstructures.h"

extern void mfx_gst_copy_srf(const mfxFrameSurface1 & dst, const mfxFrameSurface1 & src);

class BitstreamWriter
{
public:
  BitstreamWriter(mfxU8 * buf, size_t size, bool emulationControl = true);
  BitstreamWriter(mfxU8 * buf, mfxU8 * bufEnd, bool emulationControl = true);

  mfxU32 GetNumBits() const;

  void PutBit(mfxU32 bit);
  void PutBits(mfxU32 val, mfxU32 nbits);
  void PutRawBytes(mfxU8 const * begin, mfxU8 const * end); // startcode emulation is not controlled

private:
  mfxU8 * m_buf;
  mfxU8 * m_ptr;
  mfxU8 * m_bufEnd;
  mfxU32  m_bitOff;
  bool    m_emulationControl;
};

struct NalUnit
{
  NalUnit() : begin(0), end(0), type(0), numZero(0)
  {}

  NalUnit(mfxU8 * b, mfxU8 * e, mfxU8 t, mfxU8 z) : begin(b), end(e), type(t), numZero(z)
  {}

  mfxU8 * begin;
  mfxU8 * end;
  mfxU8   type;
  mfxU32  numZero;
};

NalUnit GetNalUnit(mfxU8 * begin, mfxU8 * end);

extern mfxStatus
write_nalu_length(mfxBitstream* bst);

extern mfxStatus
convert_avcc_to_bytestream(mfxBitstream* bst);

inline mfxU32 gst_video_format_to_mfx_fourcc(GstVideoFormat format)
{
  switch(format) {
    case GST_VIDEO_FORMAT_YV12:
      return MFX_FOURCC_YV12;
    case GST_VIDEO_FORMAT_I420:
      return MFX_FOURCC_I420;
    case GST_VIDEO_FORMAT_YUY2:
      return MFX_FOURCC_YUY2;
    case GST_VIDEO_FORMAT_NV12:
      return MFX_FOURCC_NV12;
    case GST_VIDEO_FORMAT_BGRA:
      return MFX_FOURCC_RGB4;
    case GST_VIDEO_FORMAT_BGRx:
      return MFX_FOURCC_RGB4;
    case GST_VIDEO_FORMAT_NV16:
      return MFX_FOURCC_NV16;
    case GST_VIDEO_FORMAT_ABGR:
      return MFX_FOURCC_BGR4;
    case GST_VIDEO_FORMAT_UYVY:
      return MFX_FOURCC_UYVY;
    case GST_VIDEO_FORMAT_ARGB64:
      return MFX_FOURCC_ARGB16;
    default:
      return 0;
  }
}

inline GstVideoFormat gst_video_format_from_mfx_fourcc(mfxU32 fourcc)
{
  switch(fourcc) {
    case MFX_FOURCC_YV12:
      return GST_VIDEO_FORMAT_YV12;
    case MFX_FOURCC_I420:
      return GST_VIDEO_FORMAT_I420;
    case MFX_FOURCC_YUY2:
      return GST_VIDEO_FORMAT_YUY2;
    case MFX_FOURCC_NV12:
      return GST_VIDEO_FORMAT_NV12;
    case MFX_FOURCC_RGB4:
      return GST_VIDEO_FORMAT_BGRA;
    case MFX_FOURCC_NV16:
      return GST_VIDEO_FORMAT_NV16;
    case MFX_FOURCC_BGR4:
      return GST_VIDEO_FORMAT_ABGR;
    case MFX_FOURCC_UYVY:
      return GST_VIDEO_FORMAT_UYVY;
    case MFX_FOURCC_ARGB16:
      return GST_VIDEO_FORMAT_ARGB64;
    default:
      return GST_VIDEO_FORMAT_UNKNOWN;
  }
}

inline const gchar * mfx_gst_interlace_mode_from_mfx_picstruct(mfxU32 fourcc)
{
  switch(fourcc) {
    case MFX_PICSTRUCT_PROGRESSIVE:
      return "progressive";
    case MFX_PICSTRUCT_FIELD_TFF:
    case MFX_PICSTRUCT_FIELD_BFF:
      return "interleaved";
    default:
      return "unknown";
  }
}

inline StreamFormat mfx_stream_format_from_fourcc(const char * stream_format)
{
  if (!stream_format) {
    return MFX_STREAM_FORMAT_UNKNOWN;
  }

  if (!strcmp(stream_format, "byte-stream")) {
    return MFX_STREAM_FORMAT_ANNEX_B;
  }
  else if (!strcmp(stream_format, "avc")) {
    return MFX_STREAM_FORMAT_AVCC;
  }
  else if (!strcmp(stream_format, "hvc1")) {
    return MFX_STREAM_FORMAT_HVC1;
  }

  return MFX_STREAM_FORMAT_UNKNOWN;
}

inline const char * mfx_stream_format_to_fourcc(StreamFormat stream_format)
{
  switch(stream_format) {
    case MFX_STREAM_FORMAT_ANNEX_B:
      return "byte-stream";
    case MFX_STREAM_FORMAT_AVCC:
      return "avc";
    case MFX_STREAM_FORMAT_HVC1:
      return "hvc1";
    default:
      return NULL;
  }
}

inline mfxU32 mfx_codecid_from_mime(const char * mime)
{
  if (!mime) {
    return 0;
  }

  if (!strcmp(mime, "video/x-h264")) {
    return MFX_CODEC_AVC;
  }
  else if (!strcmp(mime, "video/x-h265")) {
    return MFX_CODEC_HEVC;
  }
  else if (!strcmp(mime, "video/mpeg")) {
    return MFX_CODEC_MPEG2;
  }
  else if (!strcmp(mime, "image/jpeg")) {
    return MFX_CODEC_JPEG;
  }

  return 0;
}

inline const char * mfx_codecid_to_mime(mfxU32 codecid)
{
  switch(codecid) {
    case MFX_CODEC_AVC:
      return "video/x-h264";
    case MFX_CODEC_HEVC:
      return "video/x-h265";
    case MFX_CODEC_MPEG2:
      return "video/mpeg";
    case MFX_CODEC_JPEG:
      return "image/jpeg";
    default:
      return NULL;
  }
}

namespace mfx_utils {
  template<class T>
  inline T const * Begin(std::vector<T> const & t) { return &*t.begin(); }

  template<class T>
  inline T const * End(std::vector<T> const & t) { return &*t.begin() + t.size(); }

  template<class T>
  inline T * Begin(std::vector<T> & t) { return &*t.begin(); }

  template<class T>
  inline T * End(std::vector<T> & t) { return &*t.begin() + t.size(); }

  template<typename T> struct mfx_ext_buffer_id{};

  template<> struct mfx_ext_buffer_id<mfxExtVPPDeinterlacing> {
      enum { id = MFX_EXTBUFF_VPP_DEINTERLACING };
  };

  template<> struct mfx_ext_buffer_id<mfxExtVPPDenoise> {
      enum { id = MFX_EXTBUFF_VPP_DENOISE };
  };

  template<typename... Ts> struct MaxSizeof;

  template<typename T> struct MaxSizeof<T> {
    static const int max_sizeof = sizeof(T);
  };

  template<typename T, typename... Ts> struct MaxSizeof<T, Ts...> {
    static const int max_sizeof = MSDK_MAX(sizeof(T), MaxSizeof<Ts...>::max_sizeof);
  };

}

bool IsDIEnabled(const mfxVideoParam & par);

#endif // __MFX_UTILS_H__
