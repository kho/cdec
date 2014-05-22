#!/bin/sh
make distclean
#/usr/local/stow/autoconf-2.65/bin/autoreconf -if && \
autoreconf -if && \
    #./configure --with-boost=/fs/clip-software/user-supported/boost/boost-current && \
    ./configure --with-boost=/fs/clip-gpugrammar/boost_1_55_0 && \
    make -kj8
cd sa-extract && make -f Makefile.umiacs
