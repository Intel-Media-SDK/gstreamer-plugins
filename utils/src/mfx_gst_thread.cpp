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
#include "mfx_gst_thread.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxgstthread"

mfxGstThread::mfxGstThread()
  : thread_(NULL)
  , tasks_num_(0)
  , stop_thread_(false)
{
  g_rec_mutex_init(&thread_mutex_);
  g_mutex_init(&tasks_mutex_);
  g_cond_init(&tasks_cond_push_);
  g_cond_init(&tasks_cond_done_);
}

mfxGstThread::~mfxGstThread()
{
  g_cond_clear(&tasks_cond_done_);
  g_cond_clear(&tasks_cond_push_);
  g_mutex_clear(&tasks_mutex_);
  g_rec_mutex_clear(&thread_mutex_);
}

bool mfxGstThread::start()
{
  MFX_DEBUG_TRACE_FUNC;
  thread_ = gst_task_new(gst_thread_func, this, NULL);
  if (!thread_) {
    MFX_DEBUG_TRACE_MSG("failed: gst_task_new");
    return false;
  }
  gst_task_set_lock(thread_, &thread_mutex_);
  if (!gst_task_start(thread_)) {
    MFX_DEBUG_TRACE_MSG("failed: gst_task_start");
    return false;
  }
  return true;
}

void mfxGstThread::stop()
{
  MFX_DEBUG_TRACE_FUNC;
  if (thread_) {
    // setting stop_thread_ to exit the loop()
    g_mutex_lock(&tasks_mutex_);
    stop_thread_ = true;
    g_cond_signal(&tasks_cond_push_);
    g_mutex_unlock(&tasks_mutex_);

    // waiting for the thread to stop
    gst_task_join(thread_);
    gst_object_unref(thread_);
    thread_ = NULL;
  }
  // removing all cached tasks which could remain in queue
  g_mutex_lock(&tasks_mutex_);
  tasks_.clear();
  tasks_num_ = 0;
  g_cond_signal(&tasks_cond_done_); // just in case wait() was called from other thread
  g_mutex_unlock(&tasks_mutex_);
}

void mfxGstThread::wait()
{
  MFX_DEBUG_TRACE_FUNC;
  g_mutex_lock(&tasks_mutex_);
  // waiting for all tasks to be completed
  while (tasks_num_) {
    g_cond_wait(&tasks_cond_done_, &tasks_mutex_);
  }
  g_mutex_unlock(&tasks_mutex_);
}

bool mfxGstThread::post(PostTask task, TaskDoneNotify notify)
{
  MFX_DEBUG_TRACE_FUNC;
  bool res = false; // operation failure by default

  if (task) {
    g_mutex_lock(&tasks_mutex_);
    if (thread_ && !stop_thread_) {
      try {
        tasks_.push_back(Task(task, notify));
        ++tasks_num_;
        res = true;
      }
      catch(...) {
      }
      g_cond_signal(&tasks_cond_push_);
    }
    g_mutex_unlock(&tasks_mutex_);
  }
  return res;
}

mfxGstThread::Task mfxGstThread::pop()
{
  MFX_DEBUG_TRACE_FUNC;
  Task task;

  g_mutex_lock(&tasks_mutex_);
  try {
    while (!stop_thread_ && tasks_.empty()) {
      g_cond_wait(&tasks_cond_push_, &tasks_mutex_);
    }
    if (!tasks_.empty()) {
      task = tasks_.front();
      tasks_.pop_front();
    }
  }
  catch(...) {
  }
  g_mutex_unlock(&tasks_mutex_);
  return task;
}

void mfxGstThread::loop()
{
  MFX_DEBUG_TRACE_FUNC;
  Task task;

  /*while (1)*/ {
    task = pop();
    if (task.task) {
      try {
        task.task();
        if (task.notify) {
          task.notify();
        }
      }
      catch(...) {
      }
      g_mutex_lock(&tasks_mutex_);
      --tasks_num_;
      g_cond_signal(&tasks_cond_done_);
      g_mutex_unlock(&tasks_mutex_);
    }
    else {
      MFX_DEBUG_TRACE_I32(stop_thread_);
      MFX_DEBUG_TRACE_MSG("no task (we are done?)");
      //break;
    }
  }
}
