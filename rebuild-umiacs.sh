#!/bin/sh
make distclean
/fs/clip-software/autoconf/bin/autoreconf -if && \
    ./configure --prefix=$(pwd)/build --with-boost=/opt/local/toolchain/gcc48/boost-1.50.0 --with-meteor=/fs/clip-software/user-supported/meteor-1.4/meteor-1.4.jar && \
    make -kj8 && make install
make -C sa-extract clean
make -C sa-extract -kj8 -f Makefile.umiacs
