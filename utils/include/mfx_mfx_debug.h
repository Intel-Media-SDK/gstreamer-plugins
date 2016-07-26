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

#ifndef __MFX_MFX_DEBUG_H__
#define __MFX_MFX_DEBUG_H__

#include "mfx_debug.h"
#include "mfxdefs.h"

MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(mfxStatus)

#if MFX_DEBUG == MFX_DEBUG_YES

#define MFX_DEBUG_TRACE_mfxStatus(_e) printf_mfxStatus(MFX_DEBUG_TRACE_VAR, #_e, _e)

#define MFX_DEBUG_TRACE__mfxFrameInfo(_s) \
  MFX_DEBUG_TRACE_U32(_s.FourCC); \
  MFX_DEBUG_TRACE_I32(_s.Width); \
  MFX_DEBUG_TRACE_I32(_s.Height); \
  MFX_DEBUG_TRACE_I32(_s.CropX); \
  MFX_DEBUG_TRACE_I32(_s.CropY); \
  MFX_DEBUG_TRACE_I32(_s.CropW); \
  MFX_DEBUG_TRACE_I32(_s.CropH); \
  MFX_DEBUG_TRACE_I32(_s.FrameRateExtN); \
  MFX_DEBUG_TRACE_I32(_s.FrameRateExtD); \
  MFX_DEBUG_TRACE_I32(_s.AspectRatioW); \
  MFX_DEBUG_TRACE_I32(_s.AspectRatioH); \
  MFX_DEBUG_TRACE_I32(_s.PicStruct); \
  MFX_DEBUG_TRACE_I32(_s.ChromaFormat);

#define MFX_DEBUG_TRACE__mfxInfoMFX_enc_RC_method(_s) \
  MFX_DEBUG_TRACE_I32(_s.RateControlMethod); \
  if ((MFX_RATECONTROL_CBR == _s.RateControlMethod) || \
      (MFX_RATECONTROL_VBR == _s.RateControlMethod)) { \
    MFX_DEBUG_TRACE_I32(_s.InitialDelayInKB); \
    MFX_DEBUG_TRACE_I32(_s.BufferSizeInKB); \
    MFX_DEBUG_TRACE_I32(_s.TargetKbps); \
    MFX_DEBUG_TRACE_I32(_s.MaxKbps); \
  } \
  else if (MFX_RATECONTROL_CQP == _s.RateControlMethod) { \
    MFX_DEBUG_TRACE_I32(_s.QPI); \
    MFX_DEBUG_TRACE_I32(_s.BufferSizeInKB); \
    MFX_DEBUG_TRACE_I32(_s.QPP); \
    MFX_DEBUG_TRACE_I32(_s.QPB); \
  } \
  else if (MFX_RATECONTROL_AVBR == _s.RateControlMethod) { \
    MFX_DEBUG_TRACE_I32(_s.Accuracy); \
    MFX_DEBUG_TRACE_I32(_s.BufferSizeInKB); \
    MFX_DEBUG_TRACE_I32(_s.TargetKbps); \
    MFX_DEBUG_TRACE_I32(_s.Convergence); \
  } \

#define MFX_DEBUG_TRACE__mfxInfoMFX_enc(_s) \
  MFX_DEBUG_TRACE_I32(_s.LowPower); \
  MFX_DEBUG_TRACE_I32(_s.BRCParamMultiplier); \
  MFX_DEBUG_TRACE__mfxFrameInfo(_s.FrameInfo); \
  MFX_DEBUG_TRACE_U32(_s.CodecId); \
  MFX_DEBUG_TRACE_I32(_s.CodecProfile); \
  MFX_DEBUG_TRACE_I32(_s.CodecLevel); \
  MFX_DEBUG_TRACE_I32(_s.NumThread); \
  MFX_DEBUG_TRACE_I32(_s.TargetUsage); \
  MFX_DEBUG_TRACE_I32(_s.GopPicSize); \
  MFX_DEBUG_TRACE_I32(_s.GopRefDist); \
  MFX_DEBUG_TRACE_I32(_s.GopOptFlag); \
  MFX_DEBUG_TRACE_I32(_s.IdrInterval); \
  MFX_DEBUG_TRACE__mfxInfoMFX_enc_RC_method(_s); \
  MFX_DEBUG_TRACE_I32(_s.NumSlice); \
  MFX_DEBUG_TRACE_I32(_s.NumRefFrame); \
  MFX_DEBUG_TRACE_I32(_s.EncodedOrder);

#define MFX_DEBUG_TRACE_mfxVideoParam_enc(_s) \
  MFX_DEBUG_TRACE_I32(_s.AsyncDepth); \
  MFX_DEBUG_TRACE__mfxInfoMFX_enc(_s.mfx); \
  MFX_DEBUG_TRACE_I32(_s.Protected); \
  MFX_DEBUG_TRACE_U32(_s.IOPattern); \
  MFX_DEBUG_TRACE_I32(_s.NumExtParam); \
  MFX_DEBUG_TRACE_P(_s.ExtParam); \
  /*MFX_DEBUG_TRACE__mfxExtParams(_s.NumExtParam, _s.ExtParam)*/

#define MFX_DEBUG_TRACE_mfxVideoParam_vpp(_s) \
  MFX_DEBUG_TRACE_I32(_s.AsyncDepth); \
  MFX_DEBUG_TRACE__mfxFrameInfo(_s.vpp.In) \
  MFX_DEBUG_TRACE__mfxFrameInfo(_s.vpp.Out) \
  MFX_DEBUG_TRACE_I32(_s.Protected); \
  MFX_DEBUG_TRACE_U32(_s.IOPattern); \
  MFX_DEBUG_TRACE_I32(_s.NumExtParam); \
  MFX_DEBUG_TRACE_P(_s.ExtParam); \
  MFX_DEBUG_TRACE_mfxExtParams(_s.NumExtParam, _s.ExtParam)

#define MFX_DEBUG_TRACE__mfxInfoMFX_dec(_s) \
  MFX_DEBUG_TRACE__mfxFrameInfo(_s.FrameInfo); \
  MFX_DEBUG_TRACE_U32(_s.CodecId); \
  MFX_DEBUG_TRACE_I32(_s.CodecProfile); \
  MFX_DEBUG_TRACE_I32(_s.CodecLevel); \
  MFX_DEBUG_TRACE_I32(_s.NumThread); \
  MFX_DEBUG_TRACE_I32(_s.DecodedOrder); \
  MFX_DEBUG_TRACE_I32(_s.ExtendedPicStruct); \

#define MFX_DEBUG_TRACE_mfxVideoParam_dec(_s) \
  MFX_DEBUG_TRACE_I32(_s.AllocId); \
  MFX_DEBUG_TRACE_I32(_s.AsyncDepth); \
  MFX_DEBUG_TRACE__mfxInfoMFX_dec(_s.mfx) \
  MFX_DEBUG_TRACE_I32(_s.Protected); \
  MFX_DEBUG_TRACE_U32(_s.IOPattern); \
  MFX_DEBUG_TRACE_I32(_s.NumExtParam); \
  MFX_DEBUG_TRACE_P(_s.ExtParam); \

#define MFX_DEBUG_TRACE_mfxExtVPPDeinterlacing(_opt) \
  MFX_DEBUG_TRACE_I32(_opt.Mode); \
  MFX_DEBUG_TRACE_I32(_opt.TelecinePattern); \
  MFX_DEBUG_TRACE_I32(_opt.TelecineLocation);

#define MFX_DEBUG_TRACE_mfxExtVPPDenoise(_opt) \
  MFX_DEBUG_TRACE_I32(_opt.DenoiseFactor);

#define MFX_DEBUG_TRACE_mfxExtParams(_num, _params) \
  if(_params) { \
    for (mfxU32 i = 0; i < _num; ++i) { \
      switch (_params[i]->BufferId) { \
      case MFX_EXTBUFF_VPP_DEINTERLACING: \
        MFX_DEBUG_TRACE_mfxExtVPPDeinterlacing((*((mfxExtVPPDeinterlacing*)_params[i])));\
        break; \
      case MFX_EXTBUFF_VPP_DENOISE: \
        MFX_DEBUG_TRACE_mfxExtVPPDenoise((*((mfxExtVPPDenoise*)_params[i])));\
        break; \
      default: \
        MFX_DEBUG_TRACE_MSG("unknown ext buffer"); \
        MFX_DEBUG_TRACE_U32(_params[i]->BufferId); \
        break; \
      }; \
    } \
  }

#else // #if MFX_DEBUG == MFX_DEBUG_YES

#define MFX_DEBUG_TRACE_mfxStatus(_e)
#define MFX_DEBUG_TRACE__mfxFrameInfo(_s)
#define MFX_DEBUG_TRACE_mfxVideoParam_dec(_s)
#define MFX_DEBUG_TRACE_mfxVideoParam_vpp(_s)
#define MFX_DEBUG_TRACE_mfxVideoParam_enc(_s)

#endif // #if MFX_DEBUG == MFX_DEBUG_YES

#endif
