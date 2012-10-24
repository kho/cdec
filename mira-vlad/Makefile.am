bin_PROGRAMS = kbest_mirav5

kbest_mirav5_SOURCES = kbest_mirav5.cc
kbest_mirav5_LDADD = ../decoder/libcdec.a ../mteval/libmteval.a ../utils/libutils.a ../klm/search/libksearch.a ../klm/lm/libklm.a ../klm/util/libklm_util.a -lz

AM_CPPFLAGS = -W -Wall -Wno-sign-compare $(GTEST_CPPFLAGS) -I.. -I../decoder -I../mteval -I../utils -I../klm

