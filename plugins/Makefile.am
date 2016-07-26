AUTOMAKE_OPTIONS=foreign

AM_CPPFLAGS = \
  $(LIBVA_CFLAGS) \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/src/common/include \
  -I$(top_srcdir)/src/wrappers/include \
  -I$(top_srcdir)/plugins/include \
  -I$(top_srcdir)/utils/include

h264vd_sources = \
  src/mfx_gst_caps.cpp \
  src/mfx_gst_h264vd.cpp

h264ve_sources = \
  src/mfx_gst_caps.cpp \
  src/mfx_gst_h264ve.cpp

h265ve_sources = \
  src/mfx_gst_caps.cpp \
  src/mfx_gst_h265ve.cpp

jpegve_sources = \
  src/mfx_gst_caps.cpp \
  src/mfx_gst_jpegve.cpp

vpp_sources = \
  src/mfx_gst_caps.cpp \
  src/mfx_gst_vpp.cpp

lib_LTLIBRARIES =

lib_LTLIBRARIES += libmfx_gst_h264vd.la

libmfx_gst_h264vd_la_SOURCES = \
  $(h264vd_sources)
libmfx_gst_h264vd_la_DEPENDENCIES = \
  $(top_builddir)/src/common/libmfx_gst_common.la \
  $(top_builddir)/src/executors/libmfx_gst_executors.la \
  $(top_builddir)/src/wrappers/libmfx_gst_wrappers.la \
  $(top_builddir)/utils/libmfx_gst_utils.la
libmfx_gst_h264vd_la_LIBADD = \
  $(libmfx_gst_h264vd_la_DEPENDENCIES) \
  $(GST_LIBS) \
  -lgstvideo-1.0 \
  $(GST_ALLOCATORS_LIBS) \
  $(LIBVA_LIBS) $(LIBVA_DRM_LIBS) \
  -ldl
libmfx_gst_h264vd_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_h264vd_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_h264vd_la_LDFLAGS = \
  -Wl,--version-script=$(srcdir)/libmfx_gst_plugin.map \
  $(MFX_LDFLAGS) \
  $(MEDIASDK_GST_LT_LDFLAGS)

lib_LTLIBRARIES += libmfx_gst_h264ve.la

libmfx_gst_h264ve_la_SOURCES = \
  $(h264ve_sources)
libmfx_gst_h264ve_la_DEPENDENCIES = \
  $(top_builddir)/src/common/libmfx_gst_common.la \
  $(top_builddir)/src/executors/libmfx_gst_executors.la \
  $(top_builddir)/src/wrappers/libmfx_gst_wrappers.la \
  $(top_builddir)/utils/libmfx_gst_utils.la
libmfx_gst_h264ve_la_LIBADD = \
  $(libmfx_gst_h264ve_la_DEPENDENCIES) \
  $(GST_LIBS) \
  -lgstvideo-1.0 \
  $(GST_ALLOCATORS_LIBS) \
  $(LIBVA_LIBS) $(LIBVA_DRM_LIBS) \
  -ldl
libmfx_gst_h264ve_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_h264ve_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_h264ve_la_LDFLAGS = \
  -Wl,--version-script=$(srcdir)/libmfx_gst_plugin.map \
  $(MFX_LDFLAGS) \
  $(MEDIASDK_GST_LT_LDFLAGS)

lib_LTLIBRARIES += libmfx_gst_h265ve.la

libmfx_gst_h265ve_la_SOURCES = \
  $(h265ve_sources)
libmfx_gst_h265ve_la_DEPENDENCIES = \
  $(top_builddir)/src/common/libmfx_gst_common.la \
  $(top_builddir)/src/executors/libmfx_gst_executors.la \
  $(top_builddir)/src/wrappers/libmfx_gst_wrappers.la \
  $(top_builddir)/utils/libmfx_gst_utils.la
libmfx_gst_h265ve_la_LIBADD = \
  $(libmfx_gst_h265ve_la_DEPENDENCIES) \
  $(GST_LIBS) \
  -lgstvideo-1.0 \
  $(GST_ALLOCATORS_LIBS) \
  $(LIBVA_LIBS) $(LIBVA_DRM_LIBS) \
  -ldl
libmfx_gst_h265ve_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_h265ve_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_h265ve_la_LDFLAGS = \
  -Wl,--version-script=$(srcdir)/libmfx_gst_plugin.map \
  $(MFX_LDFLAGS) \
  $(MEDIASDK_GST_LT_LDFLAGS)

lib_LTLIBRARIES += libmfx_gst_jpegve.la

libmfx_gst_jpegve_la_SOURCES = \
  $(jpegve_sources)
libmfx_gst_jpegve_la_DEPENDENCIES = \
  $(top_builddir)/src/common/libmfx_gst_common.la \
  $(top_builddir)/src/executors/libmfx_gst_executors.la \
  $(top_builddir)/src/wrappers/libmfx_gst_wrappers.la \
  $(top_builddir)/utils/libmfx_gst_utils.la
libmfx_gst_jpegve_la_LIBADD = \
  $(libmfx_gst_jpegve_la_DEPENDENCIES) \
  $(GST_LIBS) \
  -lgstvideo-1.0 \
  $(GST_ALLOCATORS_LIBS) \
  $(LIBVA_LIBS) $(LIBVA_DRM_LIBS) \
  -ldl
libmfx_gst_jpegve_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_jpegve_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_jpegve_la_LDFLAGS = \
  -Wl,--version-script=$(srcdir)/libmfx_gst_plugin.map \
  $(MFX_LDFLAGS) \
  $(MEDIASDK_GST_LT_LDFLAGS)

lib_LTLIBRARIES += libmfx_gst_vpp.la

libmfx_gst_vpp_la_SOURCES = \
  $(vpp_sources)
libmfx_gst_vpp_la_DEPENDENCIES = \
  $(top_builddir)/src/common/libmfx_gst_common.la \
  $(top_builddir)/src/executors/libmfx_gst_executors.la \
  $(top_builddir)/src/wrappers/libmfx_gst_wrappers.la \
  $(top_builddir)/utils/libmfx_gst_utils.la
libmfx_gst_vpp_la_LIBADD = \
  $(libmfx_gst_vpp_la_DEPENDENCIES) \
  $(GST_LIBS) \
  -lgstvideo-1.0 \
  $(GST_ALLOCATORS_LIBS) \
  $(LIBVA_LIBS) $(LIBVA_DRM_LIBS) \
  -ldl
libmfx_gst_vpp_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_vpp_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_vpp_la_LDFLAGS = \
  -Wl,--version-script=$(srcdir)/libmfx_gst_plugin.map \
  $(MFX_LDFLAGS) \
  $(MEDIASDK_GST_LT_LDFLAGS)
