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

#ifndef __VAAPI_UTILS_H__
#define __VAAPI_UTILS_H__

#ifdef LIBVA_SUPPORT

#include <va/va.h>
#include <va/va_drmcommon.h>

#include <mfxdefs.h>

enum LibVABackend
{
    MFX_LIBVA_DRM,
};

namespace MfxLoader
{

    class SimpleLoader
    {
    public:
        SimpleLoader(const char * name);

        void * GetFunction(const char * name);

        ~SimpleLoader();

    private:
        SimpleLoader(SimpleLoader&);
        void operator=(SimpleLoader&);

        void * so_handle;
    };

#ifdef LIBVA_SUPPORT
    class VA_Proxy
    {
    private:
        SimpleLoader lib; // should appear first in member list

    public:
        typedef VAStatus (*vaInitialize_type)(VADisplay, int *, int *);
        typedef VAStatus (*vaTerminate_type)(VADisplay);
        typedef VAStatus (*vaCreateSurfaces_type)(VADisplay, unsigned int,
            unsigned int, unsigned int, VASurfaceID *, unsigned int,
            VASurfaceAttrib *, unsigned int);
        typedef VAStatus (*vaDestroySurfaces_type)(VADisplay, VASurfaceID *, int);
        typedef VAStatus (*vaCreateBuffer_type)(VADisplay, VAContextID,
            VABufferType, unsigned int, unsigned int, void *, VABufferID *);
        typedef VAStatus (*vaDestroyBuffer_type)(VADisplay, VABufferID);
        typedef VAStatus (*vaMapBuffer_type)(VADisplay, VABufferID, void **pbuf);
        typedef VAStatus (*vaUnmapBuffer_type)(VADisplay, VABufferID);
        typedef VAStatus (*vaSyncSurface_type)(VADisplay, VASurfaceID);
        typedef VAStatus (*vaDeriveImage_type)(VADisplay, VASurfaceID, VAImage *);
        typedef VAStatus (*vaDestroyImage_type)(VADisplay, VAImageID);
        typedef VAStatus (*vaGetLibFunc_type)(VADisplay, const char *func);
#ifndef DISABLE_VAAPI_BUFFER_EXPORT
        typedef VAStatus (*vaAcquireBufferHandle_type)(VADisplay, VABufferID, VABufferInfo *);
        typedef VAStatus (*vaReleaseBufferHandle_type)(VADisplay, VABufferID);
#endif

        VA_Proxy();
        ~VA_Proxy() {};

        const vaInitialize_type      vaInitialize;
        const vaTerminate_type       vaTerminate;
        const vaCreateSurfaces_type  vaCreateSurfaces;
        const vaDestroySurfaces_type vaDestroySurfaces;
        const vaCreateBuffer_type    vaCreateBuffer;
        const vaDestroyBuffer_type   vaDestroyBuffer;
        const vaMapBuffer_type       vaMapBuffer;
        const vaUnmapBuffer_type     vaUnmapBuffer;
        const vaSyncSurface_type     vaSyncSurface;
        const vaDeriveImage_type     vaDeriveImage;
        const vaDestroyImage_type    vaDestroyImage;
        const vaGetLibFunc_type      vaGetLibFunc;
#ifndef DISABLE_VAAPI_BUFFER_EXPORT
        const vaAcquireBufferHandle_type vaAcquireBufferHandle;
        const vaReleaseBufferHandle_type vaReleaseBufferHandle;
#endif
    };
#endif
} // namespace MfxLoader

mfxStatus va_to_mfx_status(VAStatus va_res);

#endif // #ifdef LIBVA_SUPPORT

#endif // #ifndef __VAAPI_UTILS_H__
