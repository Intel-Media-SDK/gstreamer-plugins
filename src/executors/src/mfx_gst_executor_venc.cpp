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

#include "mfx_gst_executor_venc.h"
#include "mfx_utils.h"
#include "vaapi_allocator.h"
#include <algorithm>

#include "mfxplugin.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstexec_venc"

mfxVersion msdk_version = { { MFX_VERSION_MINOR, MFX_VERSION_MAJOR} };
const int msdk_default_framerate = 30;

mfxGstPluginVencData::mfxGstPluginVencData(mfx_GstPlugin* plugin)
  : plg_(plugin)
  , dpyctx_(NULL)
  , impl_(MFX_IMPL_HARDWARE)
  , enc_(NULL)
  , allocator_(NULL)
  , allocator_params_(NULL)
  , enc_frame_order_(0)
  , pending_tasks_num_(0)
  , syncop_tasks_num_(0)
  , task_thread_()
  , sync_thread_()
  , is_eos_(false)
  , is_eos_done_(false)
  , stream_format_(MFX_STREAM_FORMAT_ANNEX_B)
{
  MFX_DEBUG_TRACE_FUNC;

  MSDK_ZERO_MEM(orig_frame_info_);

  // TODO define properties for these values
  enc_video_params_.mfx.FrameInfo.FrameRateExtN = msdk_default_framerate;
  enc_video_params_.mfx.FrameInfo.FrameRateExtD = 1;
  enc_video_params_.mfx.FrameInfo.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;

  for (size_t i=0; i < plg_->props_n; ++i) {
    // we walk thru all the parameters and search for any marked as 'uInit'
    if (!(plg_->props[i].usage | MfxGstPluginProperty::uInit)) continue;

    // if found, we adjust component parameter accordingly
    switch(plg_->props[i].id) {
      case PROP_GopPicSize:
        enc_video_params_.mfx.GopPicSize = plg_->props[i].vInt.def;
        break;
      case PROP_GopRefDist:
        enc_video_params_.mfx.GopRefDist = plg_->props[i].vInt.def;
        break;
      case PROP_RateControlMethod:
        enc_video_params_.mfx.RateControlMethod = plg_->props[i].vEnum.def;
        break;
      case PROP_QPI:
        enc_video_params_.mfx.QPI = plg_->props[i].vInt.def;
        break;
      case PROP_QPP:
        enc_video_params_.mfx.QPP = plg_->props[i].vInt.def;
        break;
      case PROP_QPB:
        enc_video_params_.mfx.QPB = plg_->props[i].vInt.def;
        break;
      case PROP_TargetKbps:
        enc_video_params_.mfx.TargetKbps = plg_->props[i].vInt.def;
        break;
      case PROP_MaxKbps:
        enc_video_params_.mfx.MaxKbps = plg_->props[i].vInt.def;
        break;
      case PROP_InitialDelayInKB:
        enc_video_params_.mfx.InitialDelayInKB = plg_->props[i].vInt.def;
        break;
      case PROP_BRCParamMultiplier:
        enc_video_params_.mfx.BRCParamMultiplier = plg_->props[i].vInt.def;
        break;
      case PROP_TargetUsage:
        enc_video_params_.mfx.TargetUsage = plg_->props[i].vEnum.def;
        break;
      case PROP_NumSlice:
        enc_video_params_.mfx.NumSlice = plg_->props[i].vInt.def;
        break;
      case PROP_JpegQuality:
        enc_video_params_.mfx.Quality = plg_->props[i].vInt.def;
        break;
      case PROP_AsyncDepth:
        enc_video_params_.AsyncDepth = plg_->props[i].vInt.def;
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

mfxGstPluginVencData::~mfxGstPluginVencData()
{
}

bool mfxGstPluginVencData::InitBase()
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
  /* Initialize HW device and provide handle to MSDK */
  if (impl_ == MFX_IMPL_HARDWARE) {
    dpyctx_ = mfx_gst_va_display_new();
    if (!mfx_gst_va_display_is_valid(dpyctx_)) {
      MFX_DEBUG_TRACE_MSG("failed to initialize LibVA");
      goto _error;
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

  enc_video_params_.mfx.FrameInfo.FourCC        = MFX_FOURCC_NV12;
  enc_video_params_.mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;

  enc_ = new MFXVideoENCODE(session_);
  MFX_DEBUG_TRACE_P(enc_);
  if (!enc_) {
    goto _error;
  }

  sts = session_.SetFrameAllocator(allocator_);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_MSG("failed to set allocator on mediasdk session");
    goto _error;
  }

  MFX_DEBUG_TRACE_MSG("TRUE");
  return true;

  _error:
  MSDK_DELETE(enc_);
  session_.Close();
  MSDK_DELETE(allocator_);
  MSDK_DELETE(allocator_params_);
  MSDK_OBJECT_UNREF(dpyctx_);
  MFX_DEBUG_TRACE_MSG("FALSE");
  return false;
}

bool mfxGstPluginVencData::CheckAndSetCaps(const mfxGstCapsData& caps)
{
  MFX_DEBUG_TRACE_FUNC;
  enc_video_params_.mfx.FrameInfo.Width = (mfxU16)caps.width;
  enc_video_params_.mfx.FrameInfo.Height = (mfxU16)caps.height;
  enc_video_params_.mfx.FrameInfo.CropX = (mfxU16)caps.crop_x;
  enc_video_params_.mfx.FrameInfo.CropY = (mfxU16)caps.crop_y;
  enc_video_params_.mfx.FrameInfo.CropW = (mfxU16)caps.crop_w;
  enc_video_params_.mfx.FrameInfo.CropH = (mfxU16)caps.crop_h;
  enc_video_params_.mfx.FrameInfo.FrameRateExtN = caps.framerate_n;
  enc_video_params_.mfx.FrameInfo.FrameRateExtD = caps.framerate_d;

  memcpy(&orig_frame_info_, &(enc_video_params_.mfx.FrameInfo), sizeof(mfxFrameInfo));

  // Width and height should be MB-aligned. If alignment will be bigger,
  // we may have uninitialized MB which will be encoded: as result bottom
  // artifacts will appear.
  enc_video_params_.mfx.FrameInfo.Width  = MSDK_ALIGN16(enc_video_params_.mfx.FrameInfo.Width);
  enc_video_params_.mfx.FrameInfo.Height = MSDK_ALIGN16(enc_video_params_.mfx.FrameInfo.Height);
  switch (caps.codecid) {
    case MFX_CODEC_VP8:
      enc_video_params_.mfx.CodecId = MFX_CODEC_VP8;
      enc_video_params_.mfx.CodecProfile = MFX_PROFILE_VP8_0;
      break;
    case MFX_CODEC_AVC:
      enc_video_params_.mfx.CodecId = MFX_CODEC_AVC;
      break;
    case MFX_CODEC_HEVC:
      enc_video_params_.mfx.CodecId = MFX_CODEC_HEVC;
      break;
    case MFX_CODEC_JPEG:
      enc_video_params_.mfx.CodecId = MFX_CODEC_JPEG;
      break;
    default:
      MFX_DEBUG_TRACE_MSG("bug: unsupported codecid requested: fix your code!");
      return false;
  }

  enc_video_params_.IOPattern = caps.in_memory == VIDEO_MEMORY ? MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY;
  // TODO add query call here to check parameters
  return true;
}

bool mfxGstPluginVencData::Init()
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts;
  mfxFrameAllocRequest enc_alloc_request;
  MSDK_ZERO_MEM(enc_alloc_request);

  MFX_DEBUG_TRACE_mfxVideoParam_enc(enc_video_params_);

  if (MFX_CODEC_HEVC == enc_video_params_.mfx.CodecId) {
    /* TODO: So far HEVC encoder is the only MediaSDK plug-in we use.
     * We plan to introduce a new property "PluginGuid"
     * to specify a required GUID ID in case of usage MediaSDK plug-ins.
     */
    sts = MFXVideoUSER_Load(session_, &MFX_PLUGINID_HEVCE_HW, 1);
    if (MFX_ERR_NONE != sts) {
      MFX_DEBUG_TRACE_MSG("failed to load MediaSDK plug-in");
      goto _error;
    }
  }

  sts = enc_->QueryIOSurf(&enc_video_params_, &enc_alloc_request);
  if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
    sts = MFX_ERR_NONE;
  }
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to query encoder for the required surfaces number");
    goto _error;
  }

  sts = enc_->Init(&enc_video_params_);
  if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts) {
    sts = MFX_ERR_NONE;
  }
  else if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
    sts = MFX_ERR_NONE;
  }
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to initialize encoder");
    goto _error;
  }

  sts = enc_->GetVideoParam(&enc_video_params_);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to get video params from intialized encoder");
    goto _error;
  }
  MFX_DEBUG_TRACE_mfxVideoParam_enc(enc_video_params_);

  // Some plug-ins seem to keep a surface until next one comes.
  // In this case having the only surface causes hang.
  enc_alloc_request.NumFrameSuggested = MSDK_MAX(enc_alloc_request.NumFrameSuggested, 2);

  if (!buffer_pool_.Init(enc_alloc_request.NumFrameSuggested, GetMaxBitstreamSize())) {
    MFX_DEBUG_TRACE_MSG("failed to initialize output buffer pool");
    goto _error;
  }

  return true;

  _error:
  buffer_pool_.Close();
  enc_->Close(); // we don't delete encoder here since we created it in InitBase
  if (MFX_CODEC_HEVC == enc_video_params_.mfx.CodecId) {
    MFXVideoUSER_UnLoad(session_, &MFX_PLUGINID_HEVCE_HW);
  }
  session_.Close();
  notify_error_callback_();
  return false;
}

void mfxGstPluginVencData::Dispose()
{
  MFX_DEBUG_TRACE_FUNC;
  task_thread_.stop();
  sync_thread_.stop();
  MSDK_DELETE(enc_);
  if (MFX_CODEC_HEVC == enc_video_params_.mfx.CodecId) {
    MFXVideoUSER_UnLoad(session_, &MFX_PLUGINID_HEVCE_HW);
  }
  session_.Close();
  input_locked_frames_.clear();
  /* This is a sensitive place. mfx allocator has to become a COM object.
   * The allocator is used in the frame pool which is destroyed only when all the allocated frame are returned to pool.
   * So it may happen that the frame pool will be destoyed after MSDK_DELETE(allocator_).
   * input_locked_frames_.clear() is a temporal workaround.
   */
  MSDK_DELETE(allocator_);
  MSDK_DELETE(allocator_params_);
  MSDK_OBJECT_UNREF(dpyctx_);
  buffer_pool_.Close();
}

void mfxGstPluginVencData::TaskThread_EncodeTask(
  std::shared_ptr<InputData> input_data)
{
  MFX_DEBUG_TRACE_FUNC;
/*  if (kIgnore == async_state_) {
    MSDK_WLOG << ": task ignored";
    return;
  }*/
  input_queue_.push_back(input_data);

  {
    std::unique_lock<std::mutex> lock(data_mutex_);
    --pending_tasks_num_;
  }
  cv_queue_.notify_one();

  EncodeFrameAsync();
}

void mfxGstPluginVencData::TaskThread_EosTask()
{
  MFX_DEBUG_TRACE_FUNC;
/*  if (kIgnore == async_state_) {
    MSDK_WLOG << ": task ignored";
    return;
  }*/
  is_eos_ = true;
  EncodeFrameAsync();
}

/*void mfxGstPluginVencData::AsyncThread_UseOutputBitstreamBufferTask(
  std::shared_ptr<mfxGstBitstreamBufferRef> buffer_ref)
{
  MSDK_TRACE_METHOD;
  if (kIgnore == async_state_) {
    MSDK_WLOG << ": task ignored";
    return;
  }
  output_queue_.push_back(buffer_ref.release());
  EncodeFrameAsync();
}*/

void mfxGstPluginVencData::EncodeFrameAsync()
{
  MFX_DEBUG_TRACE_FUNC;

  mfxStatus sts = MFX_ERR_NONE;
  mfxFrameSurface1 *encsrf = NULL;
  mfxBitstream* bst = NULL;

  std::shared_ptr<mfxGstVideoFrameRef> frame_ref;
  std::shared_ptr<mfxGstBitstreamBufferRef> buffer_ref;

  if (is_eos_done_) {
    MFX_DEBUG_TRACE_MSG("skipping the call: EOS already handled, we are done");
    goto _done;
  }

  while (!input_queue_.empty() || is_eos_) {
    MFX_DEBUG_TRACE_MSG("processing: got input buffer");
    buffer_ref.reset();
    frame_ref.reset();
    buffer_ref = buffer_pool_.GetBuffer();
    MFX_DEBUG_TRACE_MSG("processing: got output buffer");
    if (!buffer_ref.get()) {
      goto _done;
    }
    if (!input_queue_.empty()) {
      std::shared_ptr<InputData> input_data(input_queue_.front());
      frame_ref = input_data->frame_ref;
      //buffer_ref->ctrl.reset(input_data->ctrl.release());
      input_queue_.pop_front();
    }

    encsrf = (frame_ref.get())? frame_ref->srf(): NULL; // TODO: error handling of the map failure?
    if (encsrf) {
      encsrf->Data.FrameOrder = enc_frame_order_;
    }

    bst = buffer_ref->bst(); // TODO: error handling of the map failure?

    do {
      sts = enc_->EncodeFrameAsync(NULL/*buffer_ref->ctrl.get()*/, encsrf, bst, buffer_ref->syncp());
      if (MFX_WRN_DEVICE_BUSY == sts) {
        {
            std::unique_lock<std::mutex> lock(data_mutex_);
            cv_dev_busy_.wait(lock, [this]{ return syncop_tasks_num_ < enc_video_params_.AsyncDepth; });
        }
      }
    } while (MFX_WRN_DEVICE_BUSY == sts);

    MFX_DEBUG_TRACE_mfxStatus(sts);

    if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts) sts = MFX_ERR_NONE;
    if (MFX_ERR_NONE == sts) {
      ++enc_frame_order_;
      MFX_DEBUG_TRACE_I32(enc_frame_order_);

      ReleaseInputSurface(frame_ref);

      ++syncop_tasks_num_;
      sync_thread_.post(
        [this, buffer_ref] {
          this->mfxGstPluginVencData::SyncThread_CallSyncOperationTask(buffer_ref);
        }
      );
    }
    else if (MFX_ERR_MORE_DATA == sts) {
      if (!encsrf) {
        MFX_DEBUG_TRACE_MSG(": encoder handled EOS");
        is_eos_ = false;
        sync_thread_.post(
          std::bind(&mfxGstPluginVencData::SyncThread_NotifyEosTask,
            this));
        break;
      } else {
        MFX_DEBUG_TRACE_MSG(": encoder needs more input frames...");
        ++enc_frame_order_;
        ReleaseInputSurface(frame_ref);
      }
    }
    else {
      MFX_DEBUG_TRACE_MSG(": failed: EncodeFrameAsync");
      notify_error_callback_();
      goto _done;
    }
  }

 _done:
  return;
}

void mfxGstPluginVencData::ReleaseInputSurface(std::shared_ptr<mfxGstVideoFrameRef>& frame_ref)
{
  /* Media SDK releases input frames in the asynchronious part (in internal thread). If
   * we are lucky, we will see frame unlocked here, if we are not - next most reasonable
   * place to check will be after SyncOperation. In general we will do the following:
   *  - try to release frame right after RunFrameVPPAsync or EncodeFrameAsync
   *    * if we will fail we will submit a task to our thread to check again
   *  - once we catch a task (see SyncThread_ReleaseInputSurfaceTask) we will try again
   *    * if we will fail we will put the frame to the storage (see input_locked_frames_)
   *  - after each call to SyncOperation we will release all we can from input_locked_frames_
   */
  if (frame_ref.get()) {
    if (frame_ref->srf()->Data.Locked) {
      sync_thread_.post(
        std::bind(&mfxGstPluginVencData::SyncThread_ReleaseInputSurfaceTask,
          this,
          frame_ref));
    }
    frame_ref.reset();
  }
}

void mfxGstPluginVencData::SyncThread_ReleaseInputSurfaceTask(
  std::shared_ptr<mfxGstVideoFrameRef> frame_ref)
{
  MFX_DEBUG_TRACE_FUNC;

/*  if (kIgnore == sync_state_) {
    MSDK_WLOG << ": task ignored";
    return;
  }*/

  if (frame_ref->srf()->Data.Locked) {
    input_locked_frames_.push_back(frame_ref);
  }
}

struct mfxGstPluginVencData::CanRemoveSurface
  : public std::unary_function<std::shared_ptr<mfxGstVideoFrameRef>, bool>
{
  bool operator() (const std::shared_ptr<mfxGstVideoFrameRef>& frame_ref)
  {
    return (frame_ref->srf()->Data.Locked)? false: true;
  }
};

void mfxGstPluginVencData::SyncThread_CallSyncOperationTask(
  std::shared_ptr<mfxGstBitstreamBufferRef> buffer_ref)
{
  MFX_DEBUG_TRACE_FUNC;

/*  if (kIgnore == sync_state_) {
    MSDK_WLOG << ": task ignored";
    return;
  }*/

  mfxSyncPoint* syncp = buffer_ref->syncp();
  mfxStatus sts = session_.SyncOperation(*syncp, MSDK_TIMEOUT);

  MFX_DEBUG_TRACE_mfxStatus(sts);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_MSG(": failed: SyncOperation");
    notify_error_callback_();
    goto done;
  }

  if ((MFX_STREAM_FORMAT_AVCC == stream_format_) || (MFX_STREAM_FORMAT_HVC1 == stream_format_)) {
    mfxBitstream* bst = buffer_ref->bst();
    sts = write_nalu_length(bst);
    if (MFX_ERR_NONE != sts) {
      notify_error_callback_();
      goto done;
    }
  }
/*
  bool key_frame = false;

  switch (enc_video_params_.mfx.CodecId) {
    case MFX_CODEC_VP8:
      key_frame = buffer_ref->bst.FrameType & MFX_FRAMETYPE_I;
      break;
    case MFX_CODEC_AVC:
      key_frame = buffer_ref->bst.FrameType & MFX_FRAMETYPE_IDR;
      break;
    default:
      MSDK_ELOG << ": BUG";
      break;
  }*/

  {
    GstBuffer* buffer = buffer_ref->getBuffer();
    buffer_ref.reset();

    buffer_ready_callback_(buffer);

    // checking for unlocked surfaces and releasing them
    input_locked_frames_.erase(
      std::remove_if(
        input_locked_frames_.begin(),
        input_locked_frames_.end(),
        CanRemoveSurface()),
      input_locked_frames_.end());
  }

done:
  {
    std::unique_lock<std::mutex> lock(data_mutex_);
    --syncop_tasks_num_;
  }
  cv_dev_busy_.notify_one();
}

void mfxGstPluginVencData::SyncThread_NotifyEosTask()
{
  MFX_DEBUG_TRACE_FUNC;
  NotifyEosCallback callback = getEosCallback();
  callback();
}

bool mfxGstPluginVencData::GetProperty(
  guint id, GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_I32(id);
  switch (id) {
  case PROP_GopPicSize:
    g_value_set_int(val, enc_video_params_.mfx.GopPicSize);
    break;
  case PROP_GopRefDist:
    g_value_set_int(val, enc_video_params_.mfx.GopRefDist);
    break;
  case PROP_RateControlMethod:
    g_value_set_enum(val, enc_video_params_.mfx.RateControlMethod);
    break;
  case PROP_TargetKbps:
    if ((enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CBR) ||
        (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_VBR)) {
      g_value_set_int(val, enc_video_params_.mfx.TargetKbps);
    }
    break;
  case PROP_MaxKbps:
    if ((enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CBR) ||
        (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_VBR)) {
      g_value_set_int(val, enc_video_params_.mfx.MaxKbps);
    }
    break;
  case PROP_InitialDelayInKB:
    if ((enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CBR) ||
        (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_VBR)) {
      g_value_set_int(val, enc_video_params_.mfx.InitialDelayInKB);
    }
    break;
  case PROP_QPI:
    if (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
      g_value_set_int(val, enc_video_params_.mfx.QPI);
    }
    break;
  case PROP_QPP:
    if (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
      g_value_set_int(val, enc_video_params_.mfx.QPP);
    }
    break;
  case PROP_QPB:
    if (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
      g_value_set_int(val, enc_video_params_.mfx.QPB);
    }
    break;
  case PROP_BRCParamMultiplier:
    g_value_set_int(val, enc_video_params_.mfx.BRCParamMultiplier);
    break;
  case PROP_TargetUsage:
    g_value_set_enum(val, enc_video_params_.mfx.TargetUsage);
    break;
  case PROP_NumSlice:
    g_value_set_int(val, enc_video_params_.mfx.NumSlice);
    break;
  case PROP_JpegQuality:
    g_value_set_int(val, enc_video_params_.mfx.Quality);
    break;
  case PROP_AsyncDepth:
    g_value_set_int(val, enc_video_params_.AsyncDepth);
    break;
  case PROP_Implementation:
    g_value_set_enum(val, impl_);
    break;
  default:
    return false;
  }
  return true;
}

bool mfxGstPluginVencData::SetProperty(
  guint id, const GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_I32(id);
  switch (id) {
  case PROP_GopPicSize:
    enc_video_params_.mfx.GopPicSize = g_value_get_int(val);
    break;
  case PROP_GopRefDist:
    enc_video_params_.mfx.GopRefDist = g_value_get_int(val);
    break;
  case PROP_RateControlMethod:
    enc_video_params_.mfx.RateControlMethod = g_value_get_enum(val);
    break;
  case PROP_TargetKbps:
    if ((enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CBR) ||
        (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_VBR)) {
      enc_video_params_.mfx.TargetKbps = g_value_get_int(val);
    }
    break;
  case PROP_MaxKbps:
    if ((enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CBR) ||
        (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_VBR)) {
      enc_video_params_.mfx.MaxKbps = g_value_get_int(val);
    }
    break;
  case PROP_InitialDelayInKB:
    if ((enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CBR) ||
        (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_VBR)) {
      enc_video_params_.mfx.InitialDelayInKB = g_value_get_int(val);
    }
    break;
  case PROP_QPI:
    if (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
      enc_video_params_.mfx.QPI = g_value_get_int(val);
    }
    break;
  case PROP_QPP:
    if (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
      enc_video_params_.mfx.QPP = g_value_get_int(val);
    }
    break;
  case PROP_QPB:
    if (enc_video_params_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
      enc_video_params_.mfx.QPB = g_value_get_int(val);
    }
    break;
  case PROP_BRCParamMultiplier:
    enc_video_params_.mfx.BRCParamMultiplier = g_value_get_int(val);
    break;
  case PROP_TargetUsage:
    enc_video_params_.mfx.TargetUsage = g_value_get_enum(val);
    break;
  case PROP_NumSlice:
    enc_video_params_.mfx.NumSlice = g_value_get_int(val);
    break;
  case PROP_JpegQuality:
    enc_video_params_.mfx.Quality = g_value_get_int(val);
    break;
  case PROP_AsyncDepth:
    enc_video_params_.AsyncDepth = g_value_get_int(val);
    break;
  case PROP_Implementation:
    impl_ = g_value_get_enum(val);
    break;
  default:
    return false;
  }
#if 0
  switch (caps.codecid) {
    case MFX_CODEC_VP8:
      enc_video_params_.mfx.CodecId = MFX_CODEC_VP8;
      enc_video_params_.mfx.CodecProfile = MFX_PROFILE_VP8_0;
#if 0
      {
        int idx = enc_video_params_.enableExtParam(MFX_EXTBUFF_VP8_CODING_OPTION);
        enc_video_params_.ext_buf[idx].vp8_param.WriteIVFHeaders = MFX_CODINGOPTION_OFF;
      }
#endif
      break;
    case MFX_CODEC_AVC:
      //enc_video_params_.mfx.CodecProfile = MFX_PROFILE_AVC_HIGH;
#if 0
      {
        int idx = enc_video_params_.enableExtParam(MFX_EXTBUFF_CODING_OPTION);
        enc_video_params_.ext_buf[idx].opt.VuiNalHrdParameters = MFX_CODINGOPTION_OFF;
        enc_video_params_.ext_buf[idx].opt.PicTimingSEI        = MFX_CODINGOPTION_OFF;
        enc_video_params_.ext_buf[idx].opt.AUDelimiter         = MFX_CODINGOPTION_OFF;
      }
#endif
  }
#endif
  return true;
}

bool mfxGstPluginVencData::RunQueryContext(GstQuery *query) {
  MFX_DEBUG_TRACE_FUNC;
  const gchar *type = NULL;

  if (!query) {
    return false;
  }
  if (!gst_query_parse_context_type(query, &type)) {
    return false;
  }
  if (strcmp(type, GST_MFX_VA_DISPLAY_CONTEXT_TYPE)) {
    return false;
  }

  mfx_gst_set_context_in_query(query, dpyctx_);
  return true;
}

GstCaps* mfxGstPluginVencData::GetOutCaps(GstCaps * allowed_caps)
{
  MFX_DEBUG_TRACE_FUNC;
  GstCaps * out_caps = NULL;
  const char * mime = mfx_codecid_to_mime(enc_video_params_.mfx.CodecId);

  out_caps = gst_caps_new_simple(mime,
      "width", G_TYPE_INT, enc_video_params_.mfx.FrameInfo.Width,
      "height", G_TYPE_INT, enc_video_params_.mfx.FrameInfo.Height,
      "framerate", GST_TYPE_FRACTION,  enc_video_params_.mfx.FrameInfo.FrameRateExtN, enc_video_params_.mfx.FrameInfo.FrameRateExtD,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
      NULL);
  if (!out_caps) {
    MFX_DEBUG_TRACE_MSG("can't create encoder caps");
    return NULL;
  }

  if ((MFX_CODEC_AVC == enc_video_params_.mfx.CodecId) ||
    (MFX_CODEC_HEVC == enc_video_params_.mfx.CodecId)) {

    GstStructure * gststruct = gst_caps_get_structure(allowed_caps, 0);
    if (!gststruct) {
      MFX_DEBUG_TRACE_MSG("no caps in structure");
      return NULL;
    }

    const char * allowed_stream_format = gst_structure_get_string(gststruct, "stream-format");
    if (allowed_stream_format) {
      stream_format_ = mfx_stream_format_from_fourcc(allowed_stream_format);
    }
    if (MFX_STREAM_FORMAT_UNKNOWN == stream_format_) {
      MFX_DEBUG_TRACE_MSG("can't find proper stream-format");
      return NULL;
    }

    const char * out_stream_format = mfx_stream_format_to_fourcc(stream_format_);
    gst_caps_set_simple(out_caps, "stream-format", G_TYPE_STRING, out_stream_format, NULL);

    if (out_stream_format && (!strcmp(out_stream_format, "avc") || !strcmp(out_stream_format, "hvc1"))) {
      GstBuffer * buffer = GetCodecExtraData();
      gst_caps_set_simple(out_caps, "codec_data", GST_TYPE_BUFFER, buffer, NULL);
      gst_buffer_unref(buffer);
    }
  }

  return out_caps;
}

GstBuffer* mfxGstPluginVencData::GetCodecExtraData()
{
  MFX_DEBUG_TRACE_FUNC;
  GstBuffer * buffer = NULL;
  std::vector<mfxU8> codec_data;
  mfxU32 codec_data_size = 0;

  if (MFX_CODEC_AVC == enc_video_params_.mfx.CodecId) {
    mfxExtCodingOptionSPSPPS spspps;
    MSDK_ZERO_MEM(spspps);

    mfxExtBuffer * extBuf = &spspps.Header;

    codec_data.resize(1024);
    mfxU8 buf[1024];

    spspps.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
    spspps.Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);
    spspps.SPSBuffer = buf;
    spspps.SPSBufSize = 512;
    spspps.PPSBuffer = buf + spspps.SPSBufSize;
    spspps.PPSBufSize = 512;

    mfxVideoParam param;
    MSDK_ZERO_MEM(param);

    param.NumExtParam = 1;
    param.ExtParam = &extBuf;

    mfxStatus sts = enc_->GetVideoParam(&param);
    if (MFX_ERR_NONE != sts) {
      MFX_DEBUG_TRACE_mfxStatus(sts);
      MFX_DEBUG_TRACE_MSG("failed to get SPS/PPS from MediaSDK");
      goto _error;
    }

    {
      /* AVCDecoderConfigurationRecord:
      *  - mfxU8 - configurationVersion = 1
      *  - mfxU8 - AVCProfileIndication
      *  - mfxU8 - profile_compatibility
      *  - mfxU8 - AVCLevelIndication
      *  - mfxU8 - '111111'b + lengthSizeMinusOne
      *  - mfxU8 - '111'b + numOfSequenceParameterSets
      *  - for (i = 0; i < numOfSequenceParameterSets; ++i){
      *      - mfxU16 - sequenceParameterSetLength
      *      - mfxU8*sequenceParameterSetLength - SPS
      *    }
      *  - mfxU8 - numOfPictureParameterSets
      *  - for (i = 0; i < numOfPictureParameterSets; ++i){
      *      - mfxU16 - pictureParameterSetLength
      *      - mfxU8*pictureParameterSetLength - PPS
      *    }
      */
      BitstreamWriter bs(mfx_utils::Begin(codec_data), mfx_utils::End(codec_data), true);

      const mfxU32 configurationVersion = 0x01;
      const mfxU8 profileIdc = spspps.SPSBuffer[5];
      const mfxU8 profileCompatibility = spspps.SPSBuffer[6];
      const mfxU8 levelIdc = spspps.SPSBuffer[7];
      const mfxU8 lengthSizeMinusOne = 4 -1;

      bs.PutBits(configurationVersion, 8);
      bs.PutBits(profileIdc, 8);
      bs.PutBits(profileCompatibility, 8);
      bs.PutBits(levelIdc, 8);
      bs.PutBits(0x3F, 6);
      bs.PutBits(lengthSizeMinusOne, 2);
      bs.PutBits(0x07, 3);
      bs.PutBits(0x01, 5);

      bs.PutBits(spspps.SPSBufSize - 4, 16);
      bs.PutRawBytes(spspps.SPSBuffer + 4, spspps.SPSBuffer + spspps.SPSBufSize);

      bs.PutBits(1, 8);
      bs.PutBits(spspps.PPSBufSize - 4, 16);
      bs.PutRawBytes(spspps.PPSBuffer + 4, spspps.PPSBuffer + spspps.PPSBufSize);
      codec_data_size = bs.GetNumBits()/ 8;
    }
  }
  else if (MFX_CODEC_HEVC == enc_video_params_.mfx.CodecId) {
    mfxExtCodingOptionVPS vps;
    MSDK_ZERO_MEM(vps);
    mfxExtCodingOptionSPSPPS spspps;
    MSDK_ZERO_MEM(spspps);

    std::vector<mfxExtBuffer*> extBuf;

    extBuf.push_back((mfxExtBuffer *)&vps);
    extBuf.push_back((mfxExtBuffer *)&spspps);

    codec_data.resize(512*3);
    mfxU8 buf[512*3];

    vps.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_VPS;
    vps.Header.BufferSz = sizeof(mfxExtCodingOptionVPS);
    vps.VPSBuffer = buf;
    vps.VPSBufSize = 512;

    spspps.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
    spspps.Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);
    spspps.SPSBuffer = buf + vps.VPSBufSize;
    spspps.SPSBufSize = 512;
    spspps.PPSBuffer = buf + vps.VPSBufSize + spspps.SPSBufSize;
    spspps.PPSBufSize = 512;

    mfxVideoParam param;
    MSDK_ZERO_MEM(param);

    param.NumExtParam = (mfxU16)extBuf.size();
    param.ExtParam = &extBuf[0];

    mfxStatus sts = enc_->GetVideoParam(&param);
    if (MFX_ERR_NONE != sts) {
      MFX_DEBUG_TRACE_mfxStatus(sts);
      MFX_DEBUG_TRACE_MSG("failed to get VPS/SPS/PPS from MediaSDK");
      goto _error;
    }
    {
      /* HEVCDecoderConfigurationRecord:
      *  - mfxU8 : 8   - configurationVersion = 1
      *  - mfxU8 : 2   - general_profile_space;
      *  - mfxU8 : 1   - general_tier_flag;
      *  - mfxU8 : 5   - general_profile_idc;
      *  - mfxU32 : 32 - general_profile_compatibility_flags;
      *  - mfxU64 : 48 - general_constraint_indicator_flags;
      *  - mfxU8 : 8   - general_level_idc;
      *  - mfxU8 : 4   - reserved = ‘1111’b;
      *  - mfxU16 : 12 - min_spatial_segmentation_idc;
      *  - mfxU8 : 6   - reserved = ‘111111’b;
      *  - mfxU8 : 2   - parallelismType;
      *  - mfxU8 : 6   - reserved = ‘111111’b;
      *  - mfxU8 : 2   - chroma_format_idc;
      *  - mfxU8 : 5   - reserved = ‘11111’b;
      *  - mfxU8 : 3   - bit_depth_luma_minus8;
      *  - mfxU8 : 5   - reserved = ‘11111’b;
      *  - mfxU8 : 3   - bit_depth_chroma_minus8;
      *  - mfxU16 : 16 - avgFrameRate;
      *  - mfxU8 : 2   - constantFrameRate;
      *  - mfxU8 : 3   - numTemporalLayers;
      *  - mfxU8 : 1   - temporalIdNested;
      *  - mfxU8 : 2   - lengthSizeMinusOne;
      *  - mfxU8 : 8   - numOfArrays;
      *  - for (i=0; i < numOfArrays; ++i) {
      *  -   mfxU8 : 1   - array_completeness
      *  -   mfxU8 : 1   - reserved = 0
      *  -   mfxU8 : 6   - NAL_unit_type
      *  -   mfxU16 : 16 - numNalus
      *  -   for (j=0; j< numNalus; ++j) {
      *  -     mfxU16 - nalUnitLength
      *  -     mfxU8*nalUnitLength - nalUnit
      *  -   }
      *  - }
      */
      BitstreamWriter bs(mfx_utils::Begin(codec_data), mfx_utils::End(codec_data));

      const mfxU32 configurationVersion = 0x01;
      const mfxU32 lengthSizeMinusOne = 4 - 1;
      const mfxU32 numOfArrays = 3;

      const mfxU8 general_profile_space = 0;
      const mfxU8 general_tier_flag = !!(param.mfx.CodecLevel & MFX_TIER_HEVC_HIGH);
      const mfxU8 general_profile_idc = param.mfx.CodecProfile;
      const mfxU32 general_profile_compatibility_flags = 1 << (31 - general_profile_idc);
      const mfxU8 progressive_source_flag = !!(param.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
      const mfxU8 interlaced_source_flag = !!(param.mfx.FrameInfo.PicStruct & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF));
      const mfxU8 non_packed_constraint_flag = 0;
      const mfxU8 frame_only_constraint_flag = 0;
      const mfxU8 general_level_idc = (param.mfx.CodecLevel & 0xFF) * 3;
      const mfxU8 chroma_format_idc = param.mfx.FrameInfo.ChromaFormat;

      const mfxU32 min_spatial_segmentation_idc = 0;
      const mfxU8 parallelismType = 0;
      const mfxU16 avgFrameRate = 0;
      const mfxU8 constantFrameRate = 0;
      const mfxU8 numTemporalLayers = 0;
      const mfxU8 temporalIdNested = 0;

      const mfxU8 array_completeness = 0;

      bs.PutBits(configurationVersion, 8);
      bs.PutBits(general_profile_space, 2);
      bs.PutBit(general_tier_flag);
      bs.PutBits(general_profile_idc, 5);

      bs.PutBits((general_profile_compatibility_flags >> 8), 24);
      bs.PutBits((general_profile_compatibility_flags & 0xFF), 8);

      bs.PutBit(progressive_source_flag);
      bs.PutBit(interlaced_source_flag);
      bs.PutBit(non_packed_constraint_flag);
      bs.PutBit(frame_only_constraint_flag);
      bs.PutBits(0, 24);
      bs.PutBits(0, 20);

      bs.PutBits(general_level_idc, 8);

      bs.PutBits(0x0F, 4);  /* ‘1111’b */
      bs.PutBits(min_spatial_segmentation_idc, 12);
      bs.PutBits(0x3F, 6);  /* ‘111111’b */
      bs.PutBits(parallelismType, 2);
      bs.PutBits(0x3F, 6);  /* ‘111111’b */
      bs.PutBits(chroma_format_idc, 2);
      bs.PutBits(0x1F, 5);  /* ‘11111’b */
      bs.PutBits(0x01, 3);  /* bit_depth_luma_minus8 */
      bs.PutBits(0x1F, 5);  /* ‘11111’b */
      bs.PutBits(0x01, 3);  /* bit_depth_chroma_minus8 */
      bs.PutBits(avgFrameRate, 16);
      bs.PutBits(constantFrameRate, 2);
      bs.PutBits(numTemporalLayers, 3);
      bs.PutBits(temporalIdNested, 1);

      bs.PutBits(lengthSizeMinusOne, 2);  /* lengthSizeMinusOne */
      bs.PutBits(numOfArrays, 8);  /* numOfArrays */

      bs.PutBits(array_completeness, 1);
      bs.PutBits(0x00, 1);  /* reserved */
      bs.PutBits(0x20, 6);  /* NAL_unit_type */
      bs.PutBits(0x01, 16); /* numNalus */
      bs.PutBits(vps.VPSBufSize - 4, 16);
      bs.PutRawBytes(vps.VPSBuffer + 4, vps.VPSBuffer + vps.VPSBufSize);

      bs.PutBits(array_completeness, 1);
      bs.PutBits(0x00, 1);  /* reserved */
      bs.PutBits(0x21, 6);  /* NAL_unit_type */
      bs.PutBits(0x01, 16); /* numNalus */
      bs.PutBits(spspps.SPSBufSize - 4, 16);
      bs.PutRawBytes(spspps.SPSBuffer + 4, spspps.SPSBuffer + spspps.SPSBufSize);

      bs.PutBits(array_completeness, 1);
      bs.PutBits(0x00, 1);  /* reserved */
      bs.PutBits(0x22, 6);  /* NAL_unit_type */
      bs.PutBits(0x01, 16); /* numNalus */
      bs.PutBits(spspps.PPSBufSize - 4, 16);
      bs.PutRawBytes(spspps.PPSBuffer + 4, spspps.PPSBuffer + spspps.PPSBufSize);
      codec_data_size = bs.GetNumBits()/ 8;
    }
  }

  buffer = gst_buffer_new_and_alloc(codec_data_size);
  if (!buffer) {
    MFX_DEBUG_TRACE_MSG("failed allocate codec_data buffer");
    goto _error;
  }

  gst_buffer_fill(buffer, 0, mfx_utils::Begin(codec_data), codec_data_size);

  return buffer;

  _error:
  notify_error_callback_();
  return NULL;
}
