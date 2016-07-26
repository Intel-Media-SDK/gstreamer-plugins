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

#ifndef __MFX_DEBUG_H__
#define __MFX_DEBUG_H__

#define MFX_DEBUG_NO  0
#define MFX_DEBUG_YES 1

#define MFX_DEBUG MFX_DEBUG_NO

//#define MFX_DEBUG_STDOUT MFX_DEBUG_NO

#ifndef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "unknown"
#endif

#if MFX_DEBUG == MFX_DEBUG_YES

#include <stdio.h>
#include "mfxdefs.h"

#ifdef MFX_DEBUG_FILE_INIT
  #if MFX_DEBUG_STDOUT == MFX_DEBUG_YES
    FILE* g_dbg_file = stdout;
  #else
    #ifndef MFX_DEBUG_FILE_NAME
      #define MFX_DEBUG_FILE_NAME "mfxlog.txt"
    #endif
    FILE* g_dbg_file = fopen(MFX_DEBUG_FILE_NAME, "w");
  #endif
#else
  extern FILE* g_dbg_file;
#endif

class mfxDebugTrace
{
public:
  mfxDebugTrace(const char* _modulename, const char* _function, const char* _taskname);
  ~mfxDebugTrace(void);
  void printf_msg(const char* msg);
  void printf_i32(const char* name, mfxI32 value);
  void printf_u32(const char* name, mfxU32 value);
  void printf_i64(const char* name, mfxI64 value);
  void printf_f64(const char* name, mfxF64 value);
  void printf_p(const char* name, void* value);
  void printf_s(const char* name, const char* value);
  void printf_e(const char* name, const char* value);

protected:
  const char* modulename;
  const char* function;
  const char* taskname;

private:
  mfxDebugTrace(const mfxDebugTrace&);
  mfxDebugTrace& operator=(const mfxDebugTrace&);
};

#define MFX_DEBUG_TRACE_VAR _mfxDebugTrace

template<typename T>
struct mfxDebugValueDesc
{
  T value;
  const char* desc; // description of the value

//  inline bool operator==(const T& left, const T& right) { /* do actual comparison */ }
};

#define MFX_DEBUG_VALUE_DESC(_v) { _v, #_v }

template<typename T>
struct mfxDebugTraceValueDescCtx
{
  mfxDebugTrace& trace;
  const mfxDebugValueDesc<T>* table;
  size_t table_n;

  mfxDebugTraceValueDescCtx(
    mfxDebugTrace& _trace,
    const mfxDebugValueDesc<T>* _table,
    size_t _table_n):
      trace(_trace),
      table(_table),
      table_n(_table_n)
  {
  }
};

template<typename T>
void printf_value_from_desc(
  mfxDebugTraceValueDescCtx<T>& ctx,
  const char* name,
  T value)
{
  size_t i;
  for (i = 0; i < ctx.table_n; ++i) {
    if (value == ctx.table[i].value) {
      ctx.trace.printf_e(name, ctx.table[i].desc);
      return;
    }
  }
  // TODO alter printing function thru ctx
  ctx.trace.printf_i32(name, (mfxI32)value);
}

#define MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(TYPE,...) \
  /* declaration of the printf function*/ \
  void printf_##TYPE( \
    mfxDebugTrace& trace, \
    const char* name, \
    TYPE value);

#define MFX_DEBUG_DEFINE_VALUE_DESC_PRINTF(TYPE,...) \
  void printf_##TYPE( \
    mfxDebugTrace& trace, \
    const char* name, \
    TYPE value) \
  { \
    mfxDebugValueDesc<TYPE> t[] = \
    { \
      __VA_ARGS__ \
    }; \
    mfxDebugTraceValueDescCtx<TYPE> ctx(trace, t, sizeof(t)/sizeof(t[0])); \
    printf_value_from_desc(ctx, name, value); \
  }

#define MFX_DEBUG_TRACE(_task_name) \
  mfxDebugTrace MFX_DEBUG_TRACE_VAR(MFX_DEBUG_MODULE_NAME, __FUNCTION__, _task_name)

#define MFX_DEBUG_TRACE_FUNC \
  MFX_DEBUG_TRACE(NULL)

#define MFX_DEBUG_TRACE_MSG(_arg) \
  MFX_DEBUG_TRACE_VAR.printf_msg(_arg)

#define MFX_DEBUG_TRACE_I32(_arg) \
  MFX_DEBUG_TRACE_VAR.printf_i32(#_arg, (mfxI32)_arg)

#define MFX_DEBUG_TRACE_U32(_arg) \
  MFX_DEBUG_TRACE_VAR.printf_u32(#_arg, (mfxU32)_arg)

#define MFX_DEBUG_TRACE_I64(_arg) \
  MFX_DEBUG_TRACE_VAR.printf_i64(#_arg, (mfxI64)_arg)

#define MFX_DEBUG_TRACE_F64(_arg) \
  MFX_DEBUG_TRACE_VAR.printf_f64(#_arg, (mfxF64)_arg)

#define MFX_DEBUG_TRACE_P(_arg) \
  MFX_DEBUG_TRACE_VAR.printf_p(#_arg, (void*)_arg)

#define MFX_DEBUG_TRACE_S(_arg) \
  MFX_DEBUG_TRACE_VAR.printf_s(#_arg, _arg)

#define MFX_DEBUG_TRACE_E(_arg, _val) \
  MFX_DEBUG_TRACE_VAR.printf_e(#_arg, _val)

#define mfx_enumval_eq(_e, _v) ((_e) == _v) MFX_DEBUG_TRACE_E(_e, #_v)

#else // #if MFX_DEBUG == MFX_DEBUG_YES

#define MFX_DEBUG_TRACE_VAR

#define MFX_DEBUG_DECLARE_VALUE_DESC_PRINTF(TYPE,...)
#define MFX_DEBUG_DEFINE_VALUE_DESC_PRINTF(TYPE)

#define MFX_DEBUG_TRACE(_task_name)
#define MFX_DEBUG_TRACE_FUNC

#define MFX_DEBUG_TRACE_MSG(_arg)
#define MFX_DEBUG_TRACE_I32(_arg)
#define MFX_DEBUG_TRACE_U32(_arg)
#define MFX_DEBUG_TRACE_I64(_arg)
#define MFX_DEBUG_TRACE_F64(_arg)
#define MFX_DEBUG_TRACE_P(_arg)
#define MFX_DEBUG_TRACE_S(_arg)
#define MFX_DEBUG_TRACE_E(_arg, _val)

#endif // #if MFX_DEBUG == MFX_DEBUG_YES

#endif
