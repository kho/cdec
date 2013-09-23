#!/bin/sh
make distclean
/fs/clip-software/autoconf/bin/autoreconf -if && \
    ./configure --with-boost=/opt/local/toolchain/gcc48/boost-1.50.0 && \
    make -kj8
make -C sa-extract clean
make -C sa-extract -kj8 -f Makefile.umiacs
