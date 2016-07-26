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

#ifndef __MFX_GST_THREAD_H__
#define __MFX_GST_THREAD_H__

#include <functional>
#include <deque>

#include "mfx_gst_debug.h"

class mfxGstThread
{
public:
  typedef std::function< void() > PostTask;
  typedef std::function< void() > TaskDoneNotify;

  mfxGstThread();
  ~mfxGstThread();

  bool start();
  void stop();

  inline bool post(PostTask task) {
    TaskDoneNotify no_notify;
    return post(task, no_notify);
  }
  bool post(PostTask task, TaskDoneNotify notify);

  // Waits for completion of all submitted tasks.
  // Caller is responsible to guarantee that he will
  // not submit new tasks till completion of this call
  void wait();

protected:
  struct Task {
    Task()
    {}
    Task(PostTask task, TaskDoneNotify notify)
      : task(task)
      , notify(notify)
    {}
    PostTask task;
    TaskDoneNotify notify;
  };

  Task front();
  Task pop();

  void loop();

  static void gst_thread_func(gpointer data)
  {
    mfxGstThread* thread = (mfxGstThread*)data;
    if (thread) thread->loop();
  }

protected:

  GstTask* thread_;
  GRecMutex thread_mutex_;

  GMutex tasks_mutex_;
  GCond tasks_cond_push_;
  GCond tasks_cond_done_;

  std::deque<Task> tasks_;
  int tasks_num_; // number of not executed tasks

  bool stop_thread_;

private:
  mfxGstThread(const mfxGstThread&);
  mfxGstThread& operator=(const mfxGstThread&);
};

#endif
