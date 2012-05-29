#!/bin/sh
make clean
make distclean
autoreconf -if && \
    ./configure --with-boost=/bolt/mt/code/bin/UMD/boost-current --disable-gtest && \
    make -kj8
cd sa-extract && make -f Makefile.bolt
