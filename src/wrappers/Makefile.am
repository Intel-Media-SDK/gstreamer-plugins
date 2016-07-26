AUTOMAKE_OPTIONS=foreign

AM_CPPFLAGS = \
  $(LIBVA_CFLAGS) \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/src/wrappers/include \
  -I$(top_srcdir)/utils/include

wrappers_sources = \
  src/mfx_gst_bitstream_buffer.cpp \
  src/mfx_gst_buffer.cpp \
  src/mfx_gst_frame_constructor.cpp \
  src/mfx_gst_video_frame.cpp

noinst_LTLIBRARIES = libmfx_gst_wrappers.la

libmfx_gst_wrappers_la_SOURCES = \
  $(wrappers_sources)
libmfx_gst_wrappers_la_LIBADD = @LIBOBJS@
libmfx_gst_wrappers_la_CFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
libmfx_gst_wrappers_la_CXXFLAGS = \
  $(MFX_CFLAGS) \
  $(GST_CFLAGS)
