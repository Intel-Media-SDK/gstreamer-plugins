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

#include "mfx_gst_utils.h"

#undef MFX_DEBUG_MODULE_NAME
#define MFX_DEBUG_MODULE_NAME "gst_utils"

GType mfx_gst_get_type(MfxGstTypeDefinition& def)
{
  if (g_once_init_enter(&def.type_id)) {
    GType id = g_type_from_name(def.type_name);
    if (!id) {
      id = g_type_register_static_simple(
        def.parent,
        def.type_name,
        def.class_size,
        def.class_init,
        def.instance_size,
        def.instance_init,
        def.flags);
    }
    g_once_init_leave(&def.type_id, id);
  }
  return def.type_id;
}

bool is_dmabuf_mode(GstPad * pad)
{
  if (!pad) {
    return false;
  }
  GstElement * element = NULL;
  gchar * name = NULL;
  GstPad * peer_pad = NULL;
  bool dma_mode = false;
  gint value = 0;

  gst_object_ref(pad);

  while(1) {
    peer_pad = gst_pad_get_peer(pad);
    gst_object_unref(pad);
    if (!peer_pad) break;

    element = gst_pad_get_parent_element(peer_pad);
    gst_object_unref(peer_pad);
    if (!element) break;

    name = gst_object_get_name((GstObject*)element);
    if (!name) break;

    if (!strncmp(name, "v4l2src", 7)) {
      value = 0;
      g_object_get(element, "io-mode", &value, NULL);

      static const int V4L2SRC_IO_DMA_MODE = 4;
      dma_mode = !!(value == V4L2SRC_IO_DMA_MODE);
      break;
    }
    else if (!strncmp(name, "camerasrc", 9)) {
      value = 0;
      g_object_get(element, "io-mode", &value, NULL);

      static const int ICAMERASRC_IO_DMA_MODE = 2;
      dma_mode = !!(value == ICAMERASRC_IO_DMA_MODE);
      break;
    }
    pad = gst_element_get_static_pad(element, "sink");
    if (!pad) break;

    MSDK_G_FREE(name);
    MSDK_OBJECT_UNREF(element);
  }

  MSDK_G_FREE(name);
  MSDK_OBJECT_UNREF(element);
  return dma_mode;
}

bool is_alternate_mode(GstPad * pad)
{
  if (!pad) {
    return false;
  }
  GstElement * element = NULL;
  gchar * name = NULL;
  GstPad * peer_pad = NULL;
  bool alternate_mode = false;
  gint value = 0;

  gst_object_ref(pad);

  while(1) {
    peer_pad = gst_pad_get_peer(pad);
    gst_object_unref(pad);
    if (!peer_pad) break;

    element = gst_pad_get_parent_element(peer_pad);
    gst_object_unref(peer_pad);
    if (!element) break;

    name = gst_object_get_name((GstObject*)element);
    if (!name) break;

    if (!strncmp(name, "camerasrc", 9)) {
      value = 0;
      g_object_get(element, "interlace-mode", &value, NULL);

      static const int ICAMERASRC_ALTERNATE_MODE = 7;
      alternate_mode = !!(value == ICAMERASRC_ALTERNATE_MODE);
      break;
    }
    pad = gst_element_get_static_pad(element, "sink");
    if (!pad) break;

    MSDK_G_FREE(name);
    MSDK_OBJECT_UNREF(element);
  }

  MSDK_G_FREE(name);
  MSDK_OBJECT_UNREF(element);
  return alternate_mode;
}

bool mfx_gst_caps_contains_feature(GstCaps * caps, const char * feature)
{
  if (!caps || !feature || gst_caps_is_any(caps)) {
    return false;
  }

  for (mfxU32 i = 0; i < gst_caps_get_size(caps); ++i) {
    GstCapsFeatures* features = gst_caps_get_features(caps, i);
    if (gst_caps_features_is_any(features)) {
      continue;
    }
    if (gst_caps_features_contains(features, feature)) {
      return true;
      break;
    }
  }

  return false;
}

bool mfx_gst_peer_supports_feature(GstCaps * caps, const char * feature)
{
  if (!caps || !feature) {
    return false;
  }

  for (mfxU32 i = 0; i < gst_caps_get_size(caps); ++i) {
    GstCapsFeatures* features = gst_caps_get_features(caps, i);
    if (gst_caps_features_is_any(features)) {
      continue;
    }
    if (gst_caps_features_contains(features, feature)) {
      return true;
      break;
    }
  }
  return false;
}

const mfxU32 get_mfx_picstruct(GstBuffer* buffer, GstVideoInterlaceMode mode)
{
  mfxU32 pic_struct = MFX_PICSTRUCT_UNKNOWN;

  if (!buffer) {
    return pic_struct;
  }

  if ((mode == GST_VIDEO_INTERLACE_MODE_INTERLEAVED) || (GST_BUFFER_FLAG_IS_SET(buffer, GST_VIDEO_BUFFER_FLAG_INTERLACED))) {
    if (GST_BUFFER_FLAG_IS_SET(buffer, GST_VIDEO_BUFFER_FLAG_TFF)) {
      pic_struct = MFX_PICSTRUCT_FIELD_TFF;
    }
    else {
      pic_struct = MFX_PICSTRUCT_FIELD_BFF;
    }
  }
  else {
    pic_struct = MFX_PICSTRUCT_PROGRESSIVE;
  }

  return pic_struct;
}

const mfxU32 get_gst_picstruct(mfxU32 pic_struct)
{
  if (pic_struct & MFX_PICSTRUCT_FIELD_TFF)
    return GST_VIDEO_BUFFER_FLAG_TFF | GST_VIDEO_BUFFER_FLAG_INTERLACED;

  if (pic_struct & MFX_PICSTRUCT_FIELD_BFF)
    return GST_VIDEO_BUFFER_FLAG_INTERLACED;

  return 0;
}
