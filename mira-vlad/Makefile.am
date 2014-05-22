bin_PROGRAMS = kbest_mirav5 kbest_mirav5_discourse

kbest_mirav5_SOURCES = kbest_mirav5.cc
kbest_mirav5_LDADD = ../decoder/libcdec.a ../mteval/libmteval.a ../utils/libutils.a ../klm/search/libksearch.a ../klm/lm/libklm.a ../klm/util/libklm_util.a ../klm/util/double-conversion/libklm_util_double.a ../synutils/maxent-3.0/libtsuruoka_maxent.a -lz -lgfortran


kbest_mirav5_discourse_SOURCES = kbest_mirav5_discourse.cc
kbest_mirav5_discourse_LDADD = ../decoder/libcdec.a ../mteval/libmteval.a ../utils/libutils.a ../klm/search/libksearch.a ../klm/lm/libklm.a ../klm/util/libklm_util.a ../klm/util/double-conversion/libklm_util_double.a ../synutils/maxent-3.0/libtsuruoka_maxent.a -lz -lgfortran

AM_CPPFLAGS = -W -Wall -Wno-sign-compare $(GTEST_CPPFLAGS) -I.. -I../decoder -I../mteval -I../utils -I../klm -I../synutils

