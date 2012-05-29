#!/bin/sh
make distclean
/usr/local/stow/autoconf-2.65/bin/autoreconf -if && \
    ./configure --with-boost=/fs/clip-software/user-supported/boost/boost-current --disable-gtest && \
    make -kj8
cd sa-extract && make -f Makefile.umiacs
