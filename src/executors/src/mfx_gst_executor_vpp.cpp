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

#include "mfx_gst_executor_vpp.h"
#include "mfx_utils.h"
#include "vaapi_allocator.h"
#include <algorithm>

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstexec_vpp"

static mfxVersion msdk_version = { { MFX_VERSION_MINOR, MFX_VERSION_MAJOR} };
const int msdk_default_framerate = 30;

mfxGstPluginVppData::mfxGstPluginVppData(mfx_GstPlugin* plugin)
  : plg_(plugin)
  , dpyctx_(NULL)
  , impl_(MFX_IMPL_HARDWARE)
  , vpp_(NULL)
  , allocator_(NULL)
  , allocator_params_(NULL)
  , out_mem_type_(SYSTEM_MEMORY)
  , vpp_frame_order_(0)
  , pending_tasks_num_(0)
  , syncop_tasks_num_(0)
  , task_thread_()
  , sync_thread_()
  , deinterlacing_(FEATURE_OFF)
  , is_passthrough_(false)
  , is_eos_(false)
  , is_eos_done_(false)
{
  MFX_DEBUG_TRACE_FUNC;

  MSDK_ZERO_MEM(orig_frame_info_);

  video_params_.vpp.In.FrameRateExtN = msdk_default_framerate;
  video_params_.vpp.In.FrameRateExtD = 1;

  for (size_t i=0; i < plg_->props_n; ++i) {
    if (!(plg_->props[i].usage | MfxGstPluginProperty::uInit)) continue;

    switch(plg_->props[i].id) {
      case PROP_MemoryType:
        out_mem_type_ = (MemType)plg_->props[i].vEnum.def;
        break;
      case PROP_vpp_out_width:
        video_params_.vpp.Out.Width = plg_->props[i].vInt.def;
        break;
      case PROP_vpp_out_height:
        video_params_.vpp.Out.Height = plg_->props[i].vInt.def;
        break;
      case PROP_vpp_out_format:
        video_params_.vpp.Out.FourCC = gst_video_format_to_mfx_fourcc((GstVideoFormat)plg_->props[i].vEnum.def);
        break;
      case PROP_vpp_deinterlacing:
        deinterlacing_ = plg_->props[i].vEnum.def;
        break;
      case PROP_vpp_passthrough:
        is_passthrough_ = plg_->props[i].vEnum.def;
        break;
      case PROP_AsyncDepth:
        video_params_.AsyncDepth = plg_->props[i].vInt.def;
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

mfxGstPluginVppData::~mfxGstPluginVppData()
{
}

bool mfxGstPluginVppData::InitBase()
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

  initPar.GPUCopy = MFX_GPUCOPY_OFF;

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

  if ((impl_ == MFX_IMPL_HARDWARE) || (VIDEO_MEMORY == out_mem_type_)) {
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

  video_params_.vpp.In.ChromaFormat = video_params_.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

  vpp_ = new MFXVideoVPP(session_);
  MFX_DEBUG_TRACE_P(vpp_);
  if (!vpp_) {
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
  MSDK_DELETE(vpp_);
  session_.Close();
  MSDK_DELETE(allocator_);
  MSDK_DELETE(allocator_params_);
  MSDK_OBJECT_UNREF(dpyctx_);
  MFX_DEBUG_TRACE_MSG("FALSE");
  return false;
}

bool mfxGstPluginVppData::CheckAndSetCaps(GstCaps* caps)
{
  MFX_DEBUG_TRACE_FUNC;

  GstVideoInfo info;
  gst_video_info_init(&info);
  if (!gst_video_info_from_caps(&info, caps)) {
    return false;
  }

  video_params_.vpp.In.FourCC = gst_video_format_to_mfx_fourcc(info.finfo ? info.finfo->format : GST_VIDEO_FORMAT_UNKNOWN);
  video_params_.vpp.In.Width = info.width;
  video_params_.vpp.In.Height = is_alternate_mode(mfx_gst_plugin_get_sink_pad(plg_)) ? info.height / 2 : info.height;

  const GstStructure *gststruct = gst_caps_get_structure(caps, 0);
  if (!gststruct) {
    MFX_DEBUG_TRACE_MSG("no caps structure");
    return FALSE;
  }
  gint crop_x = 0, crop_y = 0, crop_w = 0, crop_h = 0;
  if (!gst_structure_get_int(gststruct, "crop-x", &crop_x) ||
      !gst_structure_get_int(gststruct, "crop-y", &crop_y) ||
      !gst_structure_get_int(gststruct, "crop-w", &crop_w) ||
      !gst_structure_get_int(gststruct, "crop-h", &crop_h)) {
    MFX_DEBUG_TRACE_MSG("failed to get crops: using width/height");
    crop_w = video_params_.vpp.In.Width;
    crop_h = video_params_.vpp.In.Height;
  }

  video_params_.vpp.In.CropX = crop_x;
  video_params_.vpp.In.CropY = crop_y;
  video_params_.vpp.In.CropW = crop_w;
  video_params_.vpp.In.CropH = crop_h;
  video_params_.vpp.In.FrameRateExtN = info.fps_n;
  video_params_.vpp.In.FrameRateExtD = info.fps_d;
  if (FEATURE_OFF == deinterlacing_) {
    video_params_.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
  }
  else {
    switch(deinterlacing_) {
      case MFX_DEINTERLACING_BOB:
      case MFX_DEINTERLACING_ADVANCED:
        {
          video_params_.vpp.In.PicStruct = MFX_PICSTRUCT_UNKNOWN;
          video_params_.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

          auto param = video_params_.enable<mfxExtVPPDeinterlacing>();
          if (param) param->Mode = deinterlacing_;
        }
        break;
      case MFX_DEINTERLACING_FIELD_WEAVING:
        {
          video_params_.vpp.Out.CropH = video_params_.vpp.In.CropH << 1;
          video_params_.vpp.Out.Height = MSDK_ALIGN16(video_params_.vpp.In.Height << 1);
          video_params_.vpp.Out.FrameRateExtN = video_params_.vpp.In.FrameRateExtN;
          video_params_.vpp.Out.FrameRateExtD = video_params_.vpp.In.FrameRateExtD << 1;

          video_params_.vpp.In.PicStruct = MFX_PICSTRUCT_UNKNOWN;
          video_params_.vpp.Out.PicStruct = MFX_PICSTRUCT_UNKNOWN;

          auto param = video_params_.enable<mfxExtVPPDeinterlacing>();
          if (param) param->Mode = deinterlacing_;
        }
        break;
      default:
        MFX_DEBUG_TRACE_MSG("unsupported deinterlace type");
        return false;
    }
  }

  memcpy(&orig_frame_info_, &(video_params_.vpp.In), sizeof(mfxFrameInfo));

  video_params_.vpp.In.Width = MSDK_ALIGN16(video_params_.vpp.In.Width);
  video_params_.vpp.In.Height = MSDK_ALIGN16(video_params_.vpp.In.Height);

  bool isInputVideoMemory = mfx_gst_caps_contains_feature(caps, GST_CAPS_FEATURE_MFX_FRAME_SURFACE_MEMORY) ||
    is_dmabuf_mode(mfx_gst_plugin_get_sink_pad(plg_));

  video_params_.IOPattern = 0;
  video_params_.IOPattern |= isInputVideoMemory ?
    MFX_IOPATTERN_IN_VIDEO_MEMORY :
    MFX_IOPATTERN_IN_SYSTEM_MEMORY;

  // Workaround for missing I420 support in MediaSDK
  if (MFX_FOURCC_I420 == video_params_.vpp.In.FourCC) {
    video_params_.vpp.In.FourCC = MFX_FOURCC_YV12;
  }

  if (!video_params_.vpp.Out.FourCC) {
    video_params_.vpp.Out.FourCC = video_params_.vpp.In.FourCC;
  }
  if (!video_params_.vpp.Out.Width) {
    video_params_.vpp.Out.Width = video_params_.vpp.In.Width;
  }
  if (!video_params_.vpp.Out.Height) {
    video_params_.vpp.Out.Height = video_params_.vpp.In.Height;
  }
  if (!video_params_.vpp.Out.CropW) {
    video_params_.vpp.Out.CropW = video_params_.vpp.Out.Width;
  }
  if (!video_params_.vpp.Out.CropH) {
    video_params_.vpp.Out.CropH = video_params_.vpp.Out.Height;
  }
  if (!video_params_.vpp.Out.FrameRateExtN) {
    video_params_.vpp.Out.FrameRateExtN = video_params_.vpp.In.FrameRateExtN;
  }
  if (!video_params_.vpp.Out.FrameRateExtD) {
    video_params_.vpp.Out.FrameRateExtD = video_params_.vpp.In.FrameRateExtD;
  }

  GstCaps * peer_caps = gst_pad_peer_query_caps(mfx_gst_plugin_get_src_pad(plg_), NULL);
  bool IsPeerSupportVideoMemory = false;
  if (peer_caps) {
    IsPeerSupportVideoMemory = mfx_gst_caps_contains_feature(peer_caps, GST_CAPS_FEATURE_MFX_FRAME_SURFACE_MEMORY);
    gst_caps_unref(peer_caps);
  }

  video_params_.IOPattern |= (IsPeerSupportVideoMemory && (VIDEO_MEMORY == out_mem_type_)) ?
    MFX_IOPATTERN_OUT_VIDEO_MEMORY :
    MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

  MFX_DEBUG_TRACE_I32(video_params_.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY);
  MFX_DEBUG_TRACE_I32(video_params_.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY);

  memcpy(&gst_in_video_info_, &info, sizeof(GstVideoInfo));

  MFX_DEBUG_TRACE_MSG("TRUE");
  return true;
}

bool mfxGstPluginVppData::Init()
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts;
  mfxFrameAllocRequest alloc_request[2];
  MSDK_ZERO_MEM(alloc_request[0]);
  MSDK_ZERO_MEM(alloc_request[1]);

  MFX_DEBUG_TRACE_mfxVideoParam_vpp(video_params_);

  sts = vpp_->QueryIOSurf(&video_params_, alloc_request);
  if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
    sts = MFX_ERR_NONE;
  }
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to query vpp for the required surfaces number");
    goto _error;
  }

  sts = vpp_->Init(&video_params_);
  if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts) {
    sts = MFX_ERR_NONE;
  }
  else if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
    sts = MFX_ERR_NONE;
  }
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to initialize vpp");
    goto _error;
  }

  sts = vpp_->GetVideoParam(&video_params_);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_mfxStatus(sts);
    MFX_DEBUG_TRACE_MSG("failed to get video params from intialized vpp");
    goto _error;
  }

  MFX_DEBUG_TRACE_mfxVideoParam_vpp(video_params_);

  // Some plug-ins seem to keep a surface until next one comes.
  // In this case having the only surface causes hang.
  alloc_request[1].NumFrameSuggested = MSDK_MAX(alloc_request[1].NumFrameSuggested, 2);

  if (!frame_pool_.Init(allocator_, &alloc_request[1], &gst_out_video_info_)) {
    MFX_DEBUG_TRACE_MSG("failed to initialize output buffer pool");
    goto _error;
  }

  MFX_DEBUG_TRACE_MSG("TRUE");
  return true;

  _error:
  frame_pool_.Close();
  vpp_->Close();
  session_.Close();
  notify_error_callback_();
  MFX_DEBUG_TRACE_MSG("FALSE");
  return false;
}

void mfxGstPluginVppData::Dispose()
{
  MFX_DEBUG_TRACE_FUNC;
  task_thread_.stop();
  sync_thread_.stop();
  MSDK_DELETE(vpp_);
  session_.Close();
  input_locked_frames_.clear();
  /* This is a sensitive place. mfx allocator has to become a COM object.
   * The allocator is used in the frame pool which is destroyed only when all the allocated frame are returned to pool.
   * So it may happen that the frame pool will be destoyed after MSDK_DELETE(allocator_).
   * input_locked_frames_.clear() is a temporal workaround.
   */
  frame_pool_.Close();
  MSDK_DELETE(allocator_);
  MSDK_DELETE(allocator_params_);
  MSDK_OBJECT_UNREF(dpyctx_);
}

void mfxGstPluginVppData::TaskThread_ProcessTask(
  std::shared_ptr<InputData> input_data)
{
  MFX_DEBUG_TRACE_FUNC;

  input_queue_.push_back(input_data);

  {
    std::unique_lock<std::mutex> lock(data_mutex_);
    --pending_tasks_num_;
  }
  cv_queue_.notify_one();

  ProcessTask();
}

void mfxGstPluginVppData::TaskThread_EosTask()
{
  MFX_DEBUG_TRACE_FUNC;
  is_eos_ = true;

  ProcessTask();
}

void mfxGstPluginVppData::ProcessTask_PassThrough()
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts;
  std::shared_ptr<mfxGstVideoFrameRef> in_frame_ref;
  std::shared_ptr<mfxGstVideoFrameRef> out_frame_ref;
  GstBuffer* out_buffer;

  if (!input_queue_.empty()) {
    std::shared_ptr<InputData> input_data(input_queue_.front());
    in_frame_ref = input_data->frame_ref;
    input_queue_.pop_front();
    bool copy_sys2sys = false; // NOTE add true if you want to copy system memory instead of pass ith through

    if (copy_sys2sys || (VIDEO_MEMORY == out_mem_type_)) {
      mfxFrameSurface1 *in_srf = (in_frame_ref.get())? in_frame_ref->srf(): NULL;

      std::shared_ptr<mfxGstVideoFrameRef> out_frame_ref = frame_pool_.GetBuffer();
      if (!out_frame_ref.get()) {
        MFX_DEBUG_TRACE_MSG("processing: output buffer is NULL");
        notify_error_callback_();
        goto _done;
      }
      memcpy(out_frame_ref->info(), GetOutFrameInfo(), sizeof(mfxFrameInfo));
      mfxFrameSurface1 *out_srf = out_frame_ref->srf();

      if (in_srf && out_srf) {
        if (VIDEO_MEMORY == out_mem_type_) {
          sts = allocator_->Lock(allocator_->pthis, out_srf->Data.MemId, &(out_srf->Data));
          if (MFX_ERR_NONE != sts) {
            MFX_DEBUG_TRACE_MSG(": failed: allocator_->Lock");
            notify_error_callback_();
            return;
          }
        }

        mfx_gst_copy_srf(*out_srf, *in_srf);

        if (VIDEO_MEMORY == out_mem_type_) {
          sts = allocator_->Unlock(allocator_->pthis, out_srf->Data.MemId, &(out_srf->Data));
          if (MFX_ERR_NONE != sts) {
            MFX_DEBUG_TRACE_MSG(": failed: allocator_->Unlock");
            notify_error_callback_();
            return;
          }
        }
      }
      out_buffer = out_frame_ref->getBuffer();
      out_frame_ref.reset();
    }
    else {
      out_buffer = in_frame_ref->getBuffer();
    }

    in_frame_ref.reset();
    buffer_ready_callback_(out_buffer);
  }
  else if (is_eos_) {
    sync_thread_.post(
      std::bind(&mfxGstPluginVppData::SyncThread_NotifyEosTask,
      this));
  }

_done:
  return;
}

void mfxGstPluginVppData::ProcessTask_VP()
{
  MFX_DEBUG_TRACE_FUNC;
  mfxStatus sts = MFX_ERR_NONE;
  std::shared_ptr<mfxGstVideoFrameRef> in_frame_ref;
  std::shared_ptr<mfxGstVideoFrameRef> out_frame_ref;

  while (!input_queue_.empty() || in_frame_ref.get() || is_eos_) {
    if ((MFX_ERR_NONE == sts) || (MFX_ERR_MORE_SURFACE == sts)) {
      out_frame_ref = frame_pool_.GetBuffer(); // GetBuffer is a blocking call
      if (!out_frame_ref.get()) {
        MFX_DEBUG_TRACE_MSG("processing: output buffer is NULL");
        notify_error_callback_();
        goto _done;
      }
      MFX_DEBUG_TRACE_MSG("processing: got output buffer");
    }

    if (!in_frame_ref.get() && !input_queue_.empty()) {
      MFX_DEBUG_TRACE_MSG("processing: got input buffer");
      std::shared_ptr<InputData> input_data(input_queue_.front());
      in_frame_ref = input_data->frame_ref;
      input_queue_.pop_front();
    }

    mfxFrameSurface1 *in_srf = (in_frame_ref.get())? in_frame_ref->srf(): NULL;
    if (in_srf) {
      in_srf->Data.FrameOrder = vpp_frame_order_;
    }

    memcpy(out_frame_ref->info(), GetOutFrameInfo(), sizeof(mfxFrameInfo));
    mfxFrameSurface1 *out_srf = (out_frame_ref.get())? out_frame_ref->srf(): NULL;

    mfxSyncPoint* syncp = out_frame_ref->syncp();

    if (in_srf) {
      MFX_DEBUG_TRACE__mfxFrameInfo(in_srf->Info);
    }
    if (out_srf) {
      MFX_DEBUG_TRACE__mfxFrameInfo(out_srf->Info);
    }
    do {
      sts = vpp_->RunFrameVPPAsync(in_srf, out_srf, NULL, syncp);
      if (MFX_WRN_DEVICE_BUSY == sts) {
        {
            std::unique_lock<std::mutex> lock(data_mutex_);
            cv_dev_busy_.wait(lock, [this]{ return syncop_tasks_num_ < video_params_.AsyncDepth; });
        }
      }
    } while (MFX_WRN_DEVICE_BUSY == sts);

    MFX_DEBUG_TRACE_mfxStatus(sts);

    if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts) sts = MFX_ERR_NONE;
    if ((MFX_ERR_NONE == sts) || (sts == MFX_ERR_MORE_SURFACE)) {
      ++vpp_frame_order_;
      MFX_DEBUG_TRACE_I32(vpp_frame_order_);

      if (sts == MFX_ERR_NONE) {
        ReleaseInputSurface(in_frame_ref);
      }

      ++syncop_tasks_num_;
      sync_thread_.post(
        [this, out_frame_ref] {
          this->mfxGstPluginVppData::SyncThread_CallSyncOperationTask(out_frame_ref);
        }
      );
    }
    else if (MFX_ERR_MORE_DATA == sts) {
      if (!in_srf) {
        MFX_DEBUG_TRACE_MSG(": vpp handled EOS");
        is_eos_ = false;
        sync_thread_.post(
          std::bind(&mfxGstPluginVppData::SyncThread_NotifyEosTask,
            this));
        break;
      } else {
        MFX_DEBUG_TRACE_MSG(": vpp needs more input frames...");
        ++vpp_frame_order_;
        ReleaseInputSurface(in_frame_ref);
      }
    }
    else {
      MFX_DEBUG_TRACE_MSG(": failed: RunFrameVPPAsync");
      notify_error_callback_();
      goto _done;
    }
  }
 _done:
  return;
}

void mfxGstPluginVppData::ReleaseInputSurface(std::shared_ptr<mfxGstVideoFrameRef>& frame_ref)
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
        std::bind(&mfxGstPluginVppData::SyncThread_ReleaseInputSurfaceTask,
          this,
          frame_ref));
    }
    frame_ref.reset();
  }
}

void mfxGstPluginVppData::SyncThread_ReleaseInputSurfaceTask(
  std::shared_ptr<mfxGstVideoFrameRef> frame_ref)
{
  MFX_DEBUG_TRACE_FUNC;

  if (frame_ref->srf()->Data.Locked) {
    input_locked_frames_.push_back(frame_ref);
  }
}

struct mfxGstPluginVppData::CanRemoveSurface
  : public std::unary_function<std::shared_ptr<mfxGstVideoFrameRef>, bool>
{
  bool operator() (const std::shared_ptr<mfxGstVideoFrameRef>& frame_ref)
  {
    return (frame_ref->srf()->Data.Locked)? false: true;
  }
};

void mfxGstPluginVppData::SyncThread_CallSyncOperationTask(
  std::shared_ptr<mfxGstVideoFrameRef> frame_ref)
{
  MFX_DEBUG_TRACE_FUNC;

  mfxSyncPoint* syncp = frame_ref->syncp();
  mfxStatus sts = session_.SyncOperation(*syncp, MSDK_TIMEOUT);

  MFX_DEBUG_TRACE_mfxStatus(sts);
  if (MFX_ERR_NONE != sts) {
    MFX_DEBUG_TRACE_MSG(": failed: SyncOperation");
    notify_error_callback_();
    goto done;
  }

  {
    GstBuffer* buffer = frame_ref->getBuffer();
    frame_ref.reset();

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

void mfxGstPluginVppData::SyncThread_NotifyEosTask()
{
  MFX_DEBUG_TRACE_FUNC;
  NotifyEosCallback callback = getEosCallback();
  callback();
}

bool mfxGstPluginVppData::GetProperty(
  guint id, GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_I32(id);
  switch (id) {
  case PROP_MemoryType:
    g_value_set_enum(val, out_mem_type_);
    break;
  case PROP_vpp_out_width:
    g_value_set_int(val, video_params_.vpp.Out.Width);
    break;
  case PROP_vpp_out_height:
    g_value_set_int(val, video_params_.vpp.Out.Height);
    break;
  case PROP_vpp_out_format:
    g_value_set_enum(val, gst_video_format_from_mfx_fourcc(video_params_.vpp.Out.FourCC));
    break;
  case PROP_vpp_deinterlacing:
    g_value_set_enum(val, deinterlacing_);
    break;
  case PROP_vpp_passthrough:
    g_value_set_enum(val, is_passthrough_);
    break;
  case PROP_AsyncDepth:
    g_value_set_int(val, video_params_.AsyncDepth);
    break;
  case PROP_Implementation:
    g_value_set_enum(val, impl_);
    break;
  default:
    return false;
  }
  return true;
}

bool mfxGstPluginVppData::SetProperty(
  guint id, const GValue *val, GParamSpec *ps)
{
  MFX_DEBUG_TRACE_FUNC;

  MFX_DEBUG_TRACE_I32(id);
  switch (id) {
  case PROP_MemoryType:
    out_mem_type_ = (MemType)g_value_get_enum(val);
    break;
  case PROP_vpp_out_width:
    video_params_.vpp.Out.CropW = g_value_get_int(val);
    video_params_.vpp.Out.Width = MSDK_ALIGN16(video_params_.vpp.Out.CropW);
    break;
  case PROP_vpp_out_height:
    video_params_.vpp.Out.CropH = g_value_get_int(val);
    video_params_.vpp.Out.Height = MSDK_ALIGN16(video_params_.vpp.Out.CropH);
    break;
  case PROP_vpp_out_format:
    video_params_.vpp.Out.FourCC = gst_video_format_to_mfx_fourcc((GstVideoFormat)g_value_get_enum(val));
    break;
  case PROP_vpp_deinterlacing:
    deinterlacing_ = (mfxU32)g_value_get_enum(val);
    break;
  case PROP_vpp_passthrough:
    is_passthrough_ = g_value_get_enum(val);
    break;
  case PROP_AsyncDepth:
    video_params_.AsyncDepth = g_value_get_int(val);
    break;
  case PROP_Implementation:
    impl_ = g_value_get_enum(val);
    break;
  default:
    return false;
  }

  return true;
}

GstMfxVaDisplay * mfxGstPluginVppData::QueryDisplay()
{
  MFX_DEBUG_TRACE_FUNC;
  GstContext* context = mfx_gst_query_context(&plg_->element, GST_MFX_VA_DISPLAY_CONTEXT_TYPE);
  if (context) {
    mfx_gst_get_contextdata(context, &dpyctx_);
    gst_context_unref(context);
  }
  MFX_DEBUG_TRACE_P(dpyctx_);
  return dpyctx_;
}

bool mfxGstPluginVppData::RunQueryContext(GstQuery *query)
{
  MFX_DEBUG_TRACE_FUNC;
  const gchar *type = NULL;

  if (!query) {
    MFX_DEBUG_TRACE_MSG("query objects is NULL");
    return false;
  }
  if (!gst_query_parse_context_type(query, &type)) {
    MFX_DEBUG_TRACE_MSG("can't parse query type");
    return false;
  }
  if (strcmp(type, GST_MFX_VA_DISPLAY_CONTEXT_TYPE)) {
    MFX_DEBUG_TRACE_MSG("unexpected context type");
    return false;
  }

  mfx_gst_set_context_in_query(query, dpyctx_);
  return true;
}

GstCaps* mfxGstPluginVppData::CreateOutCaps(GstCaps* incaps)
{
  MFX_DEBUG_TRACE_FUNC;
  GstCaps* out_caps = NULL;
  GstCapsFeatures* features = NULL;
  const gchar *format;
  const gchar* interlace_mode;

  GstVideoInfo info;
  gst_video_info_init(&info);

  if (is_passthrough_) {
    if (!incaps) {
      return NULL;
    }

    out_caps = gst_caps_copy(incaps);
    if (!out_caps) {
      return NULL;
    }
  }
  else {
    format = gst_video_format_to_string(gst_video_format_from_mfx_fourcc(video_params_.vpp.Out.FourCC));
    if (!format) {
      MFX_DEBUG_TRACE_MSG(": failed: can't set output format in caps");
      return NULL;
    }

    if (IsDIEnabled(video_params_)) {
      interlace_mode = mfx_gst_interlace_mode_from_mfx_picstruct(MFX_PICSTRUCT_PROGRESSIVE);
    }
    else {
      interlace_mode = mfx_gst_interlace_mode_to_string(gst_in_video_info_.interlace_mode);
    }

    out_caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, format,
      "width", G_TYPE_INT, video_params_.vpp.Out.Width,
      "height", G_TYPE_INT, video_params_.vpp.Out.Height,
      "crop-x", G_TYPE_INT, video_params_.vpp.Out.CropX,
      "crop-y", G_TYPE_INT, video_params_.vpp.Out.CropY,
      "crop-w", G_TYPE_INT, video_params_.vpp.Out.CropW,
      "crop-h", G_TYPE_INT, video_params_.vpp.Out.CropH,
      "framerate", GST_TYPE_FRACTION, video_params_.vpp.Out.FrameRateExtN, video_params_.vpp.Out.FrameRateExtD,
      "interlace-mode", G_TYPE_STRING, interlace_mode,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
      NULL);

    if (video_params_.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) {
      features = gst_caps_features_new(GST_CAPS_FEATURE_MFX_FRAME_SURFACE_MEMORY, NULL);

      if (features) {
        gst_caps_set_features(out_caps, 0, features);
      }
    }
  }

  if (!gst_video_info_from_caps(&info, out_caps)) {
    MFX_DEBUG_TRACE_MSG(": failed: can't parse out caps");
    return NULL;
  }
  memcpy(&gst_out_video_info_, &info, sizeof(GstVideoInfo));

  return out_caps;
}
