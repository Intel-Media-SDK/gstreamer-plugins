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

#ifndef __MFX_GST_EXECUTOR_VDEC_H__
#define __MFX_GST_EXECUTOR_VDEC_H__

#include <memory>
#include <vector>
#include <list>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "mfx_gst_plugin.h"
#include "mfx_gst_thread.h"
#include "mfx_gst_video_frame.h"
#include "mfx_gst_bitstream_buffer.h"
#include "mfx_gst_buffer_pool.h"
#include "mfx_gst_frame_constructor.h"
#include "mfx_gst_context.h"

#include "mfxvideo++.h"
#include "base_allocator.h"

class mfxGstPluginVdecData
{
public:
  typedef std::function< void(GstBuffer*) > BufferReadyCallback;
  typedef std::function< void() > NotifyErrorCallback;
  typedef std::function< void() > NotifyEosCallback;

  struct InputData {
    std::shared_ptr<mfxGstBitstreamBufferRef> bst_ref;
  };

  mfxGstPluginVdecData(mfx_GstPlugin* plugin);
  ~mfxGstPluginVdecData();

  // Function initializes mediasdk session, checks for HW support...
  // It performes everything which can be done knowing nothing about the
  // encoded content and settings.
  bool InitBase();
  // Function checks caps and stores them as initialization parameters
  // if they are ok.
  bool CheckAndSetCaps(GstCaps* caps);
  // Function initializes encoder with the previously accumulated parameters.
  bool Init();
  // Releases all resources, closes mediasdk session...
  // State of the object is as it was before InitBase() call.
  void Dispose();
  // Gets the property value from the component
  bool GetProperty(guint id, GValue *val, GParamSpec *ps);
  // Sets the property value from the component
  bool SetProperty(guint id, const GValue *val, GParamSpec *ps);

  bool PostDecodeTask(std::shared_ptr<InputData> input_data)
  {
    {
      std::unique_lock<std::mutex> lock(data_mutex_);
      ++pending_tasks_num_;
      cv_queue_.wait(lock, [this]{ return pending_tasks_num_ <= 1; });
    }
    return task_thread_.post(
      std::bind(
        &mfxGstPluginVdecData::TaskThread_DecodeTask,
        this,
        input_data));
  }

  bool PostEndOfStreamTask(NotifyEosCallback callback)
  {
    if (!setEosCallback(callback)) return false;
    return task_thread_.post(
      std::bind(
        &mfxGstPluginVdecData::TaskThread_EosTask,
        this));
  }

  inline void SetBufferReadyCallback(BufferReadyCallback callback) {
    buffer_ready_callback_ = callback;
  }

  inline void SetNotifyErrorCallback(NotifyErrorCallback callback) {
    notify_error_callback_ = callback;
  }

  inline mfxFrameInfo* GetFrameInfo() {
    return &dec_video_params_.mfx.FrameInfo;
  }

  inline bool StartTaskThread() {
    return task_thread_.start();
  }
  inline bool StartSyncThread() {
    return sync_thread_.start();
  }

  GstCaps* CreateOutCaps();

protected:
  // Task thread tasks
  void TaskThread_DecodeTask(std::shared_ptr<InputData> input_data);
  void TaskThread_EosTask();

  mfxStatus DecodeHeader(mfxBitstream * bst);

  void DecodeFrameAsync();

  // Sync thread tasks
  void SyncThread_CallSyncOperationTask(std::shared_ptr<mfxGstVideoFrameRef> out_frame_ref);
  void SyncThread_NotifyEosTask();

  inline bool setEosCallback(NotifyEosCallback callback) {
    if (!callback) return false;
    std::unique_lock<std::mutex> lock(data_mutex_);
    notify_eos_callback_ = callback;
    return true;
  }

  inline NotifyEosCallback getEosCallback() {
    std::unique_lock<std::mutex> lock(data_mutex_);
    NotifyEosCallback callback;
    std::swap(notify_eos_callback_, callback);
    return callback;
  }

  GstMfxVaDisplay * QueryDisplay();

protected:
  struct FindSurface
  {
    FindSurface(mfxFrameSurface1* srf) : srf_(srf) {}

    bool operator ()(const std::shared_ptr<mfxGstVideoFrameRef>& frame_ref) const
    {
        return (frame_ref->srf() == srf_);
    }

    mfxFrameSurface1* srf_;
  };

  struct CanRemoveSurface : public std::unary_function<std::shared_ptr<mfxGstVideoFrameRef>, bool>
  {
    bool operator() (const std::shared_ptr<mfxGstVideoFrameRef>& frame_ref)
    {
      return (frame_ref->srf()->Data.Locked)? false: true;
    }
  };

  BufferReadyCallback buffer_ready_callback_;
  NotifyErrorCallback notify_error_callback_;
  NotifyEosCallback notify_eos_callback_;

  mfx_GstPlugin* plg_;

  GstMfxVaDisplay *dpyctx_;
  mfxIMPL impl_;
  MFXVideoSession session_;
  MFXVideoDECODE* dec_;
  MFXFrameAllocator* allocator_;
  mfxAllocatorParams* allocator_params_;

  MemType memType_;

  MfxVideoParamWrap dec_video_params_;

  mfxU16 dec_frame_order_;
  std::atomic_uint pending_tasks_num_;
  std::atomic_uint syncop_tasks_num_;

  mfxGstThread task_thread_;
  mfxGstThread sync_thread_;

  bool is_passthrough_;
  bool is_eos_;
  bool is_eos_done_;

  std::mutex data_mutex_;
  std::condition_variable cv_queue_;
  std::condition_variable cv_dev_busy_;

  std::list<std::shared_ptr<InputData> > input_queue_;
  std::vector<std::shared_ptr<mfxGstVideoFrameRef> > locked_frames_;
  mfxGstVideoFramePoolWrap frame_pool_;
  IMfxGstFrameConstuctor * fc_;

private:
  mfxGstPluginVdecData(const mfxGstPluginVdecData&);
  mfxGstPluginVdecData& operator=(const mfxGstPluginVdecData&);
};

#endif
