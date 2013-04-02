bin_PROGRAMS = cedec

cedec_SOURCES = cedec.cc
cedec_LDADD = libcedec.a ../decoder/libcdec.a ../mteval/libmteval.a ../utils/libutils.a ../klm/search/libksearch.a ../klm/lm/libklm.a ../klm/util/libklm_util.a ../klm/util/double-conversion/libklm_util_double.a

AM_CPPFLAGS = -DBOOST_TEST_DYN_LINK -W -Wno-sign-compare -I$(top_srcdir) -I$(top_srcdir)/decoder -I$(top_srcdir)/mteval -I$(top_srcdir)/utils -I$(top_srcdir)/klm

noinst_LIBRARIES = libcedec.a

libcedec_a_SOURCES = \
app.h \
components.h \
constrain_hg.cc \
constrain_hg.h \
context.cc \
context.h \
generate_hg.cc \
generate_hg.h \
input.cc \
input.h \
inspect_hg.cc \
inspect_hg.h \
pipeline.h \
rescore_hg.cc \
rescore_hg.h \
silent.cc \
silent.h \
stages.h \
write_hg.h
