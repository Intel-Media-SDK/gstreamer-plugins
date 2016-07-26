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

#include "mfx_gst_props.h"

GParamSpec* MfxGstPluginProperty::get_param_spec(MfxGstPluginProperty* prop)
{
  switch(prop->type) {
    case G_TYPE_INT:
      return get_param_spec_int(prop);
    case G_TYPE_ENUM:
      return get_param_spec_enum(prop);
    default:
      return NULL;
  }
}

GParamSpec* MfxGstPluginProperty::get_param_spec_int(MfxGstPluginProperty* prop)
{
  return g_param_spec_int(
      prop->name, prop->nick, prop->descr,
      prop->vInt.min, prop->vInt.max, prop->vInt.def,
      prop->flags);
}

GParamSpec* MfxGstPluginProperty::get_param_spec_enum(MfxGstPluginProperty* prop)
{
  return g_param_spec_enum(
      prop->name, prop->nick, prop->descr,
      prop->vEnum.get_type(), prop->vEnum.def,
      prop->flags);
}

GType get_type__mfx_ratecontrol(void)
{
  static GType type = 0;
  const gchar *name = "MfxRateControl";
  if (!type) {
    type = g_type_from_name(name);
    if (!type) {
      static GEnumValue values[] = {
        { MFX_RATECONTROL_CBR, "Coding in CBR mode", "cbr" },
        { MFX_RATECONTROL_VBR, "Coding in VBR mode", "vbr" },
        { MFX_RATECONTROL_CQP, "Coding in CQP mode", "cqp" },
        { 0, NULL, NULL },
      };
      type = g_enum_register_static(name, values);
    }
  }
  return type;
}

GType get_type__target_usage(void)
{
  static GType type = 0;
  const gchar *name = "MfxTargetUsage";
  if (!type) {
    type = g_type_from_name(name);
    if (!type) {
      static GEnumValue values[] = {
        { MFX_TARGETUSAGE_1, "Best quality", "1" },
        { MFX_TARGETUSAGE_2, "", "2" },
        { MFX_TARGETUSAGE_3, "", "3" },
        { MFX_TARGETUSAGE_4, "Balanced", "4" },
        { MFX_TARGETUSAGE_5, "", "5" },
        { MFX_TARGETUSAGE_6, "", "6" },
        { MFX_TARGETUSAGE_7, "Best speed", "7" },
        { MFX_TARGETUSAGE_BEST_QUALITY, "Best quality", "quality" },
        { MFX_TARGETUSAGE_BALANCED, "Balanced", "balanced" },
        { MFX_TARGETUSAGE_BEST_SPEED, "Best speed", "speed" },
        { 0, NULL, NULL },
      };
      type = g_enum_register_static(name, values);
    }
  }
  return type;
}

GType get_type__memory_type(void)
{
  static GType type = 0;
  const gchar *name = "MfxOutputMemory";
  if (!type) {
    type = g_type_from_name(name);
    if (!type) {
      static GEnumValue values[] = {
        { SYSTEM_MEMORY, "Output system memory", "system" },
        { VIDEO_MEMORY, "Output video memory", "video" },
        { 0, NULL, NULL },
      };
      type = g_enum_register_static(name, values);
    }
  }
  return type;
}

GType get_type__stream_format(void)
{
  static GType type = 0;
  const gchar *name = "MfxStreamFormat";
  if (!type) {
    type = g_type_from_name(name);
    if (!type) {
      static GEnumValue values[] = {
        { MFX_STREAM_FORMAT_ANNEX_B, "Annex B format", "annexb" },
        { MFX_STREAM_FORMAT_AVCC, "AVCC format", "avcc" },
        { 0, NULL, NULL },
      };
      type = g_enum_register_static(name, values);
    }
  }
  return type;
}

GType get_type__feature_option(void)
{
  static GType type = 0;
  const gchar *name = "MfxFeatureOption";
  if (!type) {
    type = g_type_from_name(name);
    if (!type) {
      static GEnumValue values[] = {
        { FEATURE_OFF, "feature disabled", "off" },
        { FEATURE_ON, "feature enabled", "on" },
        { 0, NULL, NULL },
      };
      type = g_enum_register_static(name, values);
    }
  }
  return type;
}

GType get_type__deinterlacing(void)
{
  static GType type = 0;
  const gchar *name = "MfxDeinterlace";
  if (!type) {
    type = g_type_from_name(name);
    if (!type) {
      static GEnumValue values[] = {
        { FEATURE_OFF, "Deinterlace disabled", "off" },
        { MFX_DEINTERLACING_BOB, "Bob deinterlace", "bob" },
        { MFX_DEINTERLACING_ADVANCED, "Advanced deinterlace", "advanced" },
        { MFX_DEINTERLACING_FIELD_WEAVING, "Field weaving", "weaving" },
        { 0, NULL, NULL },
      };
      type = g_enum_register_static(name, values);
    }
  }
  return type;
}

GType get_type__implementation_type(void)
{
  static GType type = 0;
  const gchar *name = "MfxImplementation";
  if (!type) {
    type = g_type_from_name(name);
    if (!type) {
      static GEnumValue values[] = {
        { MFX_IMPL_SOFTWARE, "Pure software implementation", "sw" },
        { MFX_IMPL_HARDWARE, "Hardware accelerated implementation", "hw" },
        { 0, NULL, NULL },
      };
      type = g_enum_register_static(name, values);
    }
  }
  return type;
}