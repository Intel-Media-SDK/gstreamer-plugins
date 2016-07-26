AUTOMAKE_OPTIONS=foreign

AM_CPPFLAGS = \
  $(LIBVA_CFLAGS) \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/src/common/include \
  -I$(top_srcdir)/src/executors/include \
  -I$(top_srcdir)/src/wrappers/include \
  -I$(top_srcdir)/utils/include

#AM_CXXFLAGS = -std=c++11

executor_sources = \
  src/mfx_gst_executor_vdec.cpp \
  src/mfx_gst_executor_venc.cpp \
  src/mfx_gst_executor_vpp.cpp

noinst_LTLIBRARIES = libmfx_gst_executors.la

libmfx_gst_executors_la_SOURCES = \
  $(executor_sources)
libmfx_gst_executors_la_LIBADD = @LIBOBJS@
libmfx_gst_executors_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_executors_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
