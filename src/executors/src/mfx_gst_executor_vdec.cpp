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

#include "mfx_gst_executor_vdec.h"
#include "mfx_utils.h"
#include "vaapi_allocator.h"
#include <algorithm>
#include <memory>
#include <iostream>

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstexec_dec"

static mfxVersion msdk_version = { { MFX_VERSION_MINOR, MFX_VERSION_MAJOR} };
const int msdk_default_framerate = 30;

mfxGstPluginVdecData::mfxGstPluginVdecData(mfx_GstPlugin* plugin)
  : plg_(plugin)
  , dpyctx_(NULL)
  , impl_(MFX_IMPL_HARDWARE)
  , dec_(NULL)
  , allocator_(NULL)
  , allocator_params_(NULL)
  , memType_(SYSTEM_MEMORY)
  , dec_frame_order_(0)
  , pending_tasks_num_(0)
  , syncop_tasks_num_(0)
  , task_thread_()
  , sync_thread_()
  , is_passthrough_(false)
  , is_eos_(false)
  , is_eos_done_(false)
  , fc_(NULL)
{
  MFX_DEBUG_TRACE_FUNC;

  dec_video_params_.mfx.FrameInfo.FrameRateExtN = msdk_default_framerate;
  dec_video_params_.mfx.FrameInfo.FrameRateExtD = 1;
  dec_video_params_.mfx.FrameInfo.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;

  for (size_t i=0; i < plg_->props_n; ++i) {
    if (!(plg_->props[i].usage | MfxGstPluginProperty::uInit)) continue;

    switch(plg_->props[i].id) {
      case PROP_MemoryType:
        memType_ = (MemType)plg_->props[i].vEnum.def;
        break;
      case PROP_AsyncDepth:
        dec_video_params_.AsyncDepth = plg_->props[i].vInt.def;
        break;
      case PROP_Implementation:
        impl_ = plg_->props[i].vEnum.def;
        break;
      default:
        MFX_DEBUG_TRACE_MSG("initialization parameter ignored");
        break;
    }
  }
}

mfxGstPluginVdecData::~mfxGstPluginVdecData()
{
}

bool mfxGstPluginVdecData::InitBase()
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts;

  mfxInitParam initPar;
  MSDK_ZERO_MEM(initPar);

  mfxExtThreadsParam threadsPar;
  MSDK_ZERO_MEM(threadsPar);
  threadsPar.Header.BufferId = MFX_EXTBUFF_THREADS_PARAM;
  threadsPar.Header.BufferSz = sizeof(threadsPar);
  threadsPar.NumThread = 2;

  mfxExtBuffer* extBufs[1];
  extBufs[0] = (mfxExtBuffer*)&threadsPar;

  initPar.Version = msdk_version;
  initPar.Implementation = impl_;

  initPar.ExtParam = extBufs;
  initPar.NumExtParam = 1;

  sts = session_.InitEx(initPar);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to initialize mediasdk session");
    goto _error;
  }

  mfxIMPL impl;
  sts = session_.QueryIMPL(&impl);
  if ((MFX_ERR_NONE != sts) || (impl_ != MFX_IMPL_BASETYPE(impl))) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("initialized implemetation of mediasdk is not the expected one");
    goto _error;
  }

  if ((impl_ == MFX_IMPL_HARDWARE) || (VIDEO_MEMORY == memType_)) {
    dpyctx_ = QueryDisplay();
    if (!dpyctx_) {
      dpyctx_ = mfx_gst_va_display_new();
      if (!mfx_gst_va_display_is_valid(dpyctx_)) {
        MFX_DEBUG_TRACE_MSG("failed to initialize LibVA");
        goto _error;
      }
    }

    allocator_ = new vaapiFrameAllocator;
    MFX_DEBUG_TRACE_P(allocator_);
    if (!allocator_) {
      goto _error;
    }

    vaapiAllocatorParams *vaapiAllocParams = new vaapiAllocatorParams;
    MFX_DEBUG_TRACE_P(vaapiAllocParams);
    if (!vaapiAllocParams) {
      goto _error;
    }

    vaapiAllocParams->m_dpy = mfx_gst_va_display_get_VADisplay(dpyctx_);
    allocator_params_ = vaapiAllocParams;
    sts = allocator_->Init(allocator_params_);
    if ((MFX_ERR_NONE != sts)) {
      MFX_DEBUG_TRACE_MSG("failed to initialize allocator");
      goto _error;
    }

    sts = session_.SetHandle(MFX_HANDLE_VA_DISPLAY, mfx_gst_va_display_get_VADisplay(dpyctx_));
    if ((MFX_ERR_NONE != sts)) {
      MFX_DEBUG_TRACE_MSG("failed to set VADisplay on mediasdk session");
      goto _error;
    }
  }

  dec_video_params_.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

  dec_ = new MFXVideoDECODE(session_);
  MFX_DEBUG_TRACE_P(dec_);
  if (!dec_) {
    goto _error;
  }

  sts = session_.SetFrameAllocator(allocator_);
  if ((MFX_ERR_NONE != sts)) {
    MFX_DEBUG_TRACE_MSG("failed to set allocator on mediasdk session");
    goto _error;
  }

  MFX_DEBUG_TRACE_MSG("TRUE");
  return true;

_error:
  MSDK_DELETE(dec_);
  session_.Close();
  MSDK_DELETE(allocator_);
  MSDK_DELETE(allocator_params_);
  MSDK_OBJECT_UNREF(dpyctx_);
  MFX_DEBUG_TRACE_MSG("FALSE");
  return false;
}

bool mfxGstPluginVdecData::CheckAndSetCaps(GstCaps* caps)
{
  MFX_DEBUG_TRACE_FUNC;

  gint width, height, framerate_n, framerate_d;
  const GstStructure *gststruct;

  if (!caps || !gst_caps_is_fixed(caps)) {
    MFX_DEBUG_TRACE_MSG("no caps or they are not fixed");
    return FALSE;
  }

  gststruct = gst_caps_get_structure(caps, 0);
  if (!gststruct) {
    MFX_DEBUG_TRACE_MSG("no caps structure");
    return FALSE;
  }
  {
    gchar* str = NULL;
    MFX_DEBUG_TRACE_S(str = gst_structure_to_string(gststruct));
    if (str) g_free(str);
  }
  if (!gst_structure_get_int(gststruct, "width", &width) ||
      !gst_structure_get_int(gststruct, "height", &height) ||
      !gst_structure_get_fraction(gststruct, "framerate", &framerate_n, &framerate_d)) {
    MFX_DEBUG_TRACE_MSG("failed to get required field(s) from the caps structure");
    return FALSE;
  }

  MFX_DEBUG_TRACE_I32(width);
  MFX_DEBUG_TRACE_I32(height);
  MFX_DEBUG_TRACE_I32(framerate_n);
  MFX_DEBUG_TRACE_I32(framerate_d);

  dec_video_params_.mfx.CodecId = MFX_CODEC_AVC;
  dec_video_params_.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
  dec_video_params_.mfx.FrameInfo.Width = (mfxU16)width;
  dec_video_params_.mfx.FrameInfo.Height = (mfxU16)height;
  dec_video_params_.mfx.FrameInfo.CropW = (mfxU16)width;
  dec_video_params_.mfx.FrameInfo.CropH = (mfxU16)height;
  dec_video_params_.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;

  dec_video_params_.IOPattern = 0;
  dec_video_params_.IOPattern |= (VIDEO_MEMORY == memType_) ?
    MFX_IOPATTERN_OUT_VIDEO_MEMORY :
    MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

  fc_ = MfxGstFrameConstuctorFactory::CreateFrameConstuctor(MfxGstFC_AVC);

  const GValue * val;
  GstBuffer * buffer = NULL;
  if ((val = gst_structure_get_value(gststruct, "codec_data"))) {
    buffer = gst_value_get_buffer(val);

    std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref;
    bst_ref.reset(new mfxGstBitstreamBufferRef(buffer));

    mfxBitstream * codec_data = bst_ref.get() ? bst_ref->bst() : NULL;
    if (codec_data) {
      MfxBitstreamLoader loader(fc_);
      loader.LoadBuffer(bst_ref, true);

      mfxBitstream * bst = fc_->GetMfxBitstream();
      bst->DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;
      if (MFX_ERR_NONE != DecodeHeader(bst)) {
        MFX_DEBUG_TRACE_MSG("FALSE: can't decode codec_data");
        return false;
      }
      bst->DataFlag = 0;
    }
    else {
      MFX_DEBUG_TRACE_MSG("FALSE");
      return false;
    }

  }

  MFX_DEBUG_TRACE_MSG("TRUE");
  return true;
}

bool mfxGstPluginVdecData::Init()
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts;
  mfxFrameAllocRequest alloc_request;
  MSDK_ZERO_MEM(alloc_request);

  MFX_DEBUG_TRACE_mfxVideoParam_dec(dec_video_params_);

  sts = dec_->QueryIOSurf(&dec_video_params_, &alloc_request);
  if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
    sts = MFX_ERR_NONE;
  }
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to query vpp for the required surfaces number");
    goto _error;
  }

  if (1 == alloc_request.NumFrameSuggested) {
    // fakesink plug-in seems to keep a surface until next one comes.
    // In this case having the only surface causes hang.
    alloc_request.NumFrameSuggested += 1;
  }
  if (!frame_pool_.Init(allocator_, &alloc_request)) {
    MFX_DEBUG_TRACE_MSG("failed to initialize output buffer pool");
    goto _error;
  }

  sts = dec_->Init(&dec_video_params_);
  if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts) {
    sts = MFX_ERR_NONE;
  }
  else if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
    sts = MFX_ERR_NONE;
  }
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to initialize dec");
    goto _error;
  }

  sts = dec_->GetVideoParam(&dec_video_params_);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to get video params from intialized dec");
    goto _error;
  }

  MFX_DEBUG_TRACE_mfxVideoParam_dec(dec_video_params_);

  MFX_DEBUG_TRACE_MSG("TRUE");
  return true;

  _error:
  frame_pool_.Close();
  dec_->Close();
  session_.Close();
  notify_error_callback_();
  MFX_DEBUG_TRACE_MSG("FALSE");

  return false;
}

void mfxGstPluginVdecData::Dispose()
{
  MFX_DEBUG_TRACE_FUNC;
  task_thread_.stop();
  sync_thread_.stop();
  MSDK_DELETE(dec_);
  session_.Close();
  MSDK_DELETE(fc_);
  locked_frames_.clear();
  frame_pool_.Close();
  MSDK_DELETE(allocator_);
  MSDK_DELETE(allocator_params_);
  MSDK_OBJECT_UNREF(dpyctx_);
}

void mfxGstPluginVdecData::TaskThread_DecodeTask(
  std::shared_ptr<InputData> input_data)
{
  MFX_DEBUG_TRACE_FUNC;

  input_queue_.push_back(input_data);

  {
    std::unique_lock<std::mutex> lock(data_mutex_);
    --pending_tasks_num_;
  }
  cv_queue_.notify_one();

  DecodeFrameAsync();
}

void mfxGstPluginVdecData::TaskThread_EosTask()
{
  MFX_DEBUG_TRACE_FUNC;
  is_eos_ = true;

  DecodeFrameAsync();
}

void mfxGstPluginVdecData::DecodeFrameAsync()
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts = MFX_ERR_NONE;
  std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref;
  std::shared_ptr<mfxGstVideoFrameRef> frame_ref;

  mfxBitstream * bst = fc_->GetMfxBitstream();
  if (bst && !bst->Data) {
    sts = MFX_ERR_MORE_DATA;
  }

  MfxBitstreamLoader loader(fc_);

  while ((MFX_ERR_NONE == sts) || (MFX_ERR_MORE_DATA == sts)) {
    bst_ref.reset();
    frame_ref.reset();

    if (MFX_ERR_MORE_DATA == sts) {
      if (!input_queue_.empty()) {
        std::shared_ptr<InputData> input_data(input_queue_.front());
        bst_ref = input_data->bst_ref;
        input_queue_.pop_front();
        loader.LoadBuffer(bst_ref, false);
      }
      bst = fc_->GetMfxBitstream();
      if (!bst->DataLength) {
        if (is_eos_) {
          bst = NULL;
        }
        else {
          break;
        }
      }
    }

    MFX_DEBUG_TRACE_MSG("processing: got input buffer");

    frame_ref = frame_pool_.GetBuffer();
    if (!frame_ref.get()) {
      MFX_DEBUG_TRACE_MSG("error: output buffer is NULL");
      notify_error_callback_();
      goto _done;
    }
    MFX_DEBUG_TRACE_MSG("processing: got output buffer");

    memcpy(frame_ref->info(), GetFrameInfo(), sizeof(mfxFrameInfo));
    mfxFrameSurface1 * work_srf = frame_ref->srf();
    {
      std::unique_lock<std::mutex> lock(data_mutex_);
      locked_frames_.push_back(frame_ref);
    }

    mfxSyncPoint * free_syncp = frame_ref->syncp();

    mfxFrameSurface1 * out_srf = NULL;

    do {
      sts = dec_->DecodeFrameAsync(bst, work_srf, &out_srf, free_syncp);
      if (MFX_WRN_DEVICE_BUSY == sts) {
        {
          std::unique_lock<std::mutex> lock(data_mutex_);
          cv_dev_busy_.wait(lock, [this]{ return syncop_tasks_num_ < dec_video_params_.AsyncDepth; });
        }
      }
    } while (MFX_WRN_DEVICE_BUSY == sts);

    MFX_DEBUG_TRACE_mfxStatus(sts);

    if (MFX_WRN_VIDEO_PARAM_CHANGED == sts) {
      sts = MFX_ERR_MORE_SURFACE;
    }
    if (MFX_ERR_NONE == sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts) {
      if (out_srf) {
        ++dec_frame_order_;

        std::shared_ptr<mfxGstVideoFrameRef> out_frame_ref;

        {
          std::unique_lock<std::mutex> lock(data_mutex_);

          std::vector<std::shared_ptr<mfxGstVideoFrameRef> >::iterator it= std::find_if(
            locked_frames_.begin(),
            locked_frames_.end(),
            FindSurface(out_srf));
          if (it != locked_frames_.end()) {
            out_frame_ref = *it;
            mfxSyncPoint * out_syncp = out_frame_ref->syncp();
            *out_syncp = *free_syncp;
          }
          else {
            MFX_DEBUG_TRACE_MSG("error: can't find cached mfxGstVideoFrameRef object");
            notify_error_callback_();
            goto _done;
          }
        }

        ++syncop_tasks_num_;

        sync_thread_.post(
          [this, out_frame_ref] {
            this->mfxGstPluginVdecData::SyncThread_CallSyncOperationTask(out_frame_ref);
          }
        );
      }
      else if (!bst) {
        MFX_DEBUG_TRACE_MSG(": dec handled EOS");
        is_eos_ = false;
        sync_thread_.post(
          std::bind(&mfxGstPluginVdecData::SyncThread_NotifyEosTask,
            this));
      }
    }
    else {
      MFX_DEBUG_TRACE_MSG(": failed: DecodeFrameAsync");
      notify_error_callback_();
      goto _done;
    }

    {
      std::unique_lock<std::mutex> lock(data_mutex_);
      locked_frames_.erase(
        std::remove_if(
          locked_frames_.begin(),
          locked_frames_.end(),
          CanRemoveSurface()),
        locked_frames_.end());
    }
  }

  {
    std::unique_lock<std::mutex> lock(data_mutex_);
    locked_frames_.erase(
      std::remove_if(
        locked_frames_.begin(),
        locked_frames_.end(),
        CanRemoveSurface()),
      locked_frames_.end());
  }

 _done:
  return;
}

void mfxGstPluginVdecData::SyncThread_CallSyncOperationTask(
  std::shared_ptr<mfxGstVideoFrameRef> out_frame_ref)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxSyncPoint* syncp = out_frame_ref->syncp();
  mfxStatus sts = session_.SyncOperation(*syncp, MSDK_TIMEOUT);

  MFX_DEBUG_TRACE_mfxStatus(sts);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_MSG(": failed: SyncOperation");
    notify_error_callback_();
    goto done;
  }

  {
    GstBuffer* buffer = out_frame_ref->getBuffer();

    buffer_ready_callback_(buffer);

    {
      std::unique_lock<std::mutex> lock(data_mutex_);
      locked_frames_.erase(
        std::remove_if(
          locked_frames_.begin(),
          locked_frames_.end(),
          FindSurface(out_frame_ref->srf())),
        locked_frames_.end());
    }
    out_frame_ref.reset();
  }

done:
  {
    std::unique_lock<std::mutex> lock(data_mutex_);
    --syncop_tasks_num_;
  }
  cv_dev_busy_.notify_one();
}

void mfxGstPluginVdecData::SyncThread_NotifyEosTask()
{
  MFX_DEBUG_TRACE_FUNC;
  NotifyEosCallback callback = getEosCallback();
  callback();
}

bool mfxGstPluginVdecData::GetProperty(
  guint id, GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_I32(id);
  switch (id) {
  case PROP_MemoryType:
    g_value_set_enum(val, memType_);
    break;
  case PROP_AsyncDepth:
    g_value_set_int(val, dec_video_params_.AsyncDepth);
    break;
  case PROP_Implementation:
    g_value_set_enum(val, impl_);
    break;
  default:
    return false;
  }
  return true;
}

bool mfxGstPluginVdecData::SetProperty(
  guint id, const GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_I32(id);
  switch (id) {
  case PROP_MemoryType:
    memType_ = (MemType)g_value_get_enum(val);
    break;
  case PROP_AsyncDepth:
    dec_video_params_.AsyncDepth = g_value_get_int(val);
    break;
  case PROP_Implementation:
    impl_ = g_value_get_enum(val);
    break;
  default:
    return false;
  }

  return true;
}

GstMfxVaDisplay * mfxGstPluginVdecData::QueryDisplay() {

  GstContext* context = mfx_gst_query_context(&plg_->element, GST_MFX_VA_DISPLAY_CONTEXT_TYPE);
  if (context) {
    mfx_gst_get_contextdata(context, &dpyctx_);
    gst_context_unref(context);
  }
  return dpyctx_;
}

GstCaps* mfxGstPluginVdecData::CreateOutCaps() {
  MFX_DEBUG_TRACE_FUNC;
  GstCaps* out_caps;
  GstCapsFeatures* features;
  const gchar* format;
  const gchar* interlace_mode;

  format = gst_video_format_to_string(gst_video_format_from_mfx_fourcc(dec_video_params_.mfx.FrameInfo.FourCC));
  if (!format) {
    MFX_DEBUG_TRACE_MSG(": failed: can't find output format for caps");
    return NULL;
  }

  interlace_mode = mfx_gst_interlace_mode_from_mfx_picstruct(dec_video_params_.mfx.FrameInfo.PicStruct);

  out_caps = gst_caps_new_simple("video/x-raw",
    "format", G_TYPE_STRING, format,
    "width", G_TYPE_INT, dec_video_params_.mfx.FrameInfo.Width,
    "height", G_TYPE_INT, dec_video_params_.mfx.FrameInfo.Height,
    "crop-x", G_TYPE_INT, dec_video_params_.mfx.FrameInfo.CropX,
    "crop-y", G_TYPE_INT, dec_video_params_.mfx.FrameInfo.CropY,
    "crop-w", G_TYPE_INT, dec_video_params_.mfx.FrameInfo.CropW,
    "crop-h", G_TYPE_INT, dec_video_params_.mfx.FrameInfo.CropH,
    "framerate", GST_TYPE_FRACTION, dec_video_params_.mfx.FrameInfo.FrameRateExtN, dec_video_params_.mfx.FrameInfo.FrameRateExtD,
    "interlace-mode", G_TYPE_STRING, interlace_mode,
    "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
    NULL);

  if (!out_caps) {
    MFX_DEBUG_TRACE_MSG(": failed: can't create output caps");
    return NULL;
  }

  if (VIDEO_MEMORY == memType_) {
    // FIXME : at first we need query downstream plugin whether it supports custom memory otherwise we will crash
    features = gst_caps_features_new(GST_CAPS_FEATURE_MFX_FRAME_SURFACE_MEMORY, NULL);

    if (features) {
      gst_caps_set_features(out_caps, 0, features);
    }
  }

  return out_caps;
}

mfxStatus mfxGstPluginVdecData::DecodeHeader(mfxBitstream * bst)
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts = dec_->DecodeHeader(bst, &dec_video_params_);
  return sts;
}
