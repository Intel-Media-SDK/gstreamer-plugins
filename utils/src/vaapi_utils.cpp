/**********************************************************************************

Copyright (C) 2005-2019 Intel Corporation.  All rights reserved.

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

#ifdef LIBVA_SUPPORT

#include "vaapi_utils.h"
#include <dlfcn.h>
#include <stdexcept>

namespace MfxLoader
{

SimpleLoader::SimpleLoader(const char * name)
{
    so_handle = dlopen(name, RTLD_GLOBAL | RTLD_NOW);
}

void * SimpleLoader::GetFunction(const char * name)
{
    void * fn_ptr = dlsym(so_handle, name);
    if (!fn_ptr)
        throw std::runtime_error("Can't find function");
    return fn_ptr;
}

SimpleLoader::~SimpleLoader()
{
    dlclose(so_handle);
}

#define SIMPLE_LOADER_STRINGIFY1( x) #x
#define SIMPLE_LOADER_STRINGIFY(x) SIMPLE_LOADER_STRINGIFY1(x)
#define SIMPLE_LOADER_DECORATOR1(fun,suffix) fun ## _ ## suffix
#define SIMPLE_LOADER_DECORATOR(fun,suffix) SIMPLE_LOADER_DECORATOR1(fun,suffix)


// Following macro applied on vaInitialize will give:  vaInitialize((vaInitialize_type)lib.GetFunction("vaInitialize"))
#define SIMPLE_LOADER_FUNCTION(name) name( (SIMPLE_LOADER_DECORATOR(name, type)) lib.GetFunction(SIMPLE_LOADER_STRINGIFY(name)) )


#if defined(LIBVA_SUPPORT)
VA_Proxy::VA_Proxy()
    : lib("libva.so.1")
    , SIMPLE_LOADER_FUNCTION(vaInitialize)
    , SIMPLE_LOADER_FUNCTION(vaTerminate)
    , SIMPLE_LOADER_FUNCTION(vaCreateSurfaces)
    , SIMPLE_LOADER_FUNCTION(vaDestroySurfaces)
    , SIMPLE_LOADER_FUNCTION(vaCreateBuffer)
    , SIMPLE_LOADER_FUNCTION(vaDestroyBuffer)
    , SIMPLE_LOADER_FUNCTION(vaMapBuffer)
    , SIMPLE_LOADER_FUNCTION(vaUnmapBuffer)
    , SIMPLE_LOADER_FUNCTION(vaSyncSurface)
    , SIMPLE_LOADER_FUNCTION(vaDeriveImage)
    , SIMPLE_LOADER_FUNCTION(vaDestroyImage)
    , SIMPLE_LOADER_FUNCTION(vaGetLibFunc)
#ifndef DISABLE_VAAPI_BUFFER_EXPORT
    , SIMPLE_LOADER_FUNCTION(vaAcquireBufferHandle)
    , SIMPLE_LOADER_FUNCTION(vaReleaseBufferHandle)
#endif
{
}

#endif

#undef SIMPLE_LOADER_FUNCTION

} // MfxLoader

mfxStatus va_to_mfx_status(VAStatus va_res)
{
    mfxStatus mfxRes = MFX_ERR_NONE;

    switch (va_res)
    {
    case VA_STATUS_SUCCESS:
        mfxRes = MFX_ERR_NONE;
        break;
    case VA_STATUS_ERROR_ALLOCATION_FAILED:
        mfxRes = MFX_ERR_MEMORY_ALLOC;
        break;
    case VA_STATUS_ERROR_ATTR_NOT_SUPPORTED:
    case VA_STATUS_ERROR_UNSUPPORTED_PROFILE:
    case VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT:
    case VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT:
    case VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE:
    case VA_STATUS_ERROR_FLAG_NOT_SUPPORTED:
    case VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED:
        mfxRes = MFX_ERR_UNSUPPORTED;
        break;
    case VA_STATUS_ERROR_INVALID_DISPLAY:
    case VA_STATUS_ERROR_INVALID_CONFIG:
    case VA_STATUS_ERROR_INVALID_CONTEXT:
    case VA_STATUS_ERROR_INVALID_SURFACE:
    case VA_STATUS_ERROR_INVALID_BUFFER:
    case VA_STATUS_ERROR_INVALID_IMAGE:
    case VA_STATUS_ERROR_INVALID_SUBPICTURE:
        mfxRes = MFX_ERR_NOT_INITIALIZED;
        break;
    case VA_STATUS_ERROR_INVALID_PARAMETER:
        mfxRes = MFX_ERR_INVALID_VIDEO_PARAM;
    default:
        mfxRes = MFX_ERR_UNKNOWN;
        break;
    }
    return mfxRes;
}

#endif // #ifdef LIBVA_SUPPORT
