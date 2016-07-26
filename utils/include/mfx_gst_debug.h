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

#ifndef __MFX_GST_DEBUG_H__
#define __MFX_GST_DEBUG_H__

#include "mfx_debug.h"
#include <gst/gst.h>
#include <gst/gstelement.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>

MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(gboolean)
MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(GstPadDirection)
MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(GstPadPresence)
MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(GstStateChange)
MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(GstStateChangeReturn)
MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(GstFlowReturn)
MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(GstEventType)

#if MFX_DEBUG == MFX_DEBUG_YES

#define MFX_DEBUG_TRACE__gboolean(_e) printf_gboolean(MFX_DEBUG_TRACE_VAR, #_e, _e)
#define MFX_DEBUG_TRACE__GstPadDirection(_e) printf_GstPadDirection(MFX_DEBUG_TRACE_VAR, #_e, _e)
#define MFX_DEBUG_TRACE__GstPadPresence(_e) printf_GstPadPresence(MFX_DEBUG_TRACE_VAR, #_e, _e)
#define MFX_DEBUG_TRACE__GstStateChange(_e) printf_GstStateChange(MFX_DEBUG_TRACE_VAR, #_e, _e)
#define MFX_DEBUG_TRACE__GstStateChangeReturn(_e) printf_GstStateChangeReturn(MFX_DEBUG_TRACE_VAR, #_e, _e)
#define MFX_DEBUG_TRACE__GstFlowReturn(_e) printf_GstFlowReturn(MFX_DEBUG_TRACE_VAR, #_e, _e)
#define MFX_DEBUG_TRACE__GstEventType(_e) printf_GstEventType(MFX_DEBUG_TRACE_VAR, #_e, _e)

#else // #if MFX_DEBUG == MFX_DEBUG_YES

#define MFX_DEBUG_TRACE__gboolean(_e)
#define MFX_DEBUG_TRACE__GstPadDirection(_e)
#define MFX_DEBUG_TRACE__GstPadPresence(_e)
#define MFX_DEBUG_TRACE__GstStateChange(_e)
#define MFX_DEBUG_TRACE__GstStateChangeReturn(_e)
#define MFX_DEBUG_TRACE__GstFlowReturn(_e)
#define MFX_DEBUG_TRACE__GstEventType(_e)

#endif // #if MFX_DEBUG == MFX_DEBUG_YES

#define MFX_DEBUG_TRACE__GstStaticCaps(_s) \
  MFX_DEBUG_TRACE_S(_s.string);

#define MFX_DEBUG_TRACE__GstStaticPadTemplate(_s) \
  MFX_DEBUG_TRACE_S(_s.name_template); \
  MFX_DEBUG_TRACE__GstPadDirection(_s.direction); \
  MFX_DEBUG_TRACE__GstPadPresence(_s.presence); \
  MFX_DEBUG_TRACE__GstStaticCaps(_s.static_caps);

#endif
