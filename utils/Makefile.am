AUTOMAKE_OPTIONS=foreign

AM_CPPFLAGS = \
  $(LIBVA_CFLAGS) \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/utils/include

utils_sources = \
  src/mfx_debug.cpp \
  src/mfx_gst_buffer_pool.cpp \
  src/mfx_gst_context.cpp \
  src/mfx_gst_debug.cpp \
  src/mfx_gst_props.cpp \
  src/mfx_gst_thread.cpp \
  src/mfx_gst_utils.cpp \
  src/mfx_gst_video_frame_pool.cpp \
  src/mfx_gst_video_memory.cpp \
  src/mfx_mfx_debug.cpp \
  src/mfx_utils.cpp \
  src/mfx_vaapi_debug.cpp \
  src/base_allocator.cpp \
  src/vaapi_allocator.cpp \
  src/vaapi_utils.cpp

noinst_LTLIBRARIES = libmfx_gst_utils.la

libmfx_gst_utils_la_SOURCES = \
  $(utils_sources)
libmfx_gst_utils_la_LIBADD = @LIBOBJS@
libmfx_gst_utils_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_utils_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
