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

#include <string.h>

#include "mfx_debug.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "mfxdebug"

#if MFX_DEBUG == MFX_DEBUG_YES

static const char* g_debug_pattern[] =
{
};

static bool is_matched(const char* str)
{
  if (0 == sizeof(g_debug_pattern)/sizeof(g_debug_pattern[0])) return true; // match all
  if (!str) return false;

  for (size_t i=0; i < sizeof(g_debug_pattern)/sizeof(g_debug_pattern[0]); ++i) {
    if (strstr(str, g_debug_pattern[i])) return true;
  }
  return false;
}

mfxDebugTrace::mfxDebugTrace(
  const char* _modulename,
  const char* _function,
  const char* _taskname)
{
  modulename = _modulename;
  function = _function;
  taskname = _taskname;

  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: +\n", modulename, function, taskname);
    else
      fprintf(g_dbg_file, "%s: %s: +\n", modulename, function);
    fflush(g_dbg_file);
  }
}

mfxDebugTrace::~mfxDebugTrace(void)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: -\n", modulename, function, taskname);
    else
      fprintf(g_dbg_file, "%s: %s: -\n", modulename, function);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_i32(const char* name, mfxI32 value)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: %s = %d\n", modulename, function, taskname, name, value);
    else
      fprintf(g_dbg_file, "%s: %s: %s = %d\n", modulename, function, name, value);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_u32(const char* name, mfxU32 value)
{
  if (!is_matched(function)) return;
  if (g_dbg_file)
  {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: %s = 0x%x\n", modulename, function, taskname, name, value);
    else
      fprintf(g_dbg_file, "%s: %s: %s = 0x%x\n", modulename, function, name, value);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_i64(const char* name, mfxI64 value)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: %s = %lld\n", modulename, function, taskname, name, value);
    else
      fprintf(g_dbg_file, "%s: %s: %s = %lld\n", modulename, function, name, value);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_f64(const char* name, mfxF64 value)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: %s = %f\n", modulename, function, taskname, name, value);
    else
      fprintf(g_dbg_file, "%s: %s: %s = %f\n", modulename, function, name, value);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_p(const char* name, void* value)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: %s = %p\n", modulename, function, taskname, name, value);
    else
      fprintf(g_dbg_file, "%s: %s: %s = %p\n", modulename, function, name, value);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_s(const char* name, const char* value)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: %s = '%s'\n", modulename, function, taskname, name, value);
    else
      fprintf(g_dbg_file, "%s: %s: %s = '%s'\n", modulename, function, name, value);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_e(const char* name, const char* value)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s: %s: %s = %s\n", modulename, function, taskname, name, value);
    else
      fprintf(g_dbg_file, "%s: %s: %s = %s\n", modulename, function, name, value);
    fflush(g_dbg_file);
  }
}

void mfxDebugTrace::printf_msg(const char* msg)
{
  if (!is_matched(function)) return;
  if (g_dbg_file) {
    if (taskname)
      fprintf(g_dbg_file, "%s: %s %s: %s\n", modulename, function, taskname, msg);
    else
      fprintf(g_dbg_file, "%s: %s %s\n", modulename, function, msg);
    fflush(g_dbg_file);
  }
}

#endif // #if MFX_DEBUG == MFX_DEBUG_YES
