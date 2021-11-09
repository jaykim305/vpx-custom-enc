#!/bin/bash

cd ./libvpx
bash ./configure --target=x86_64-linux-gcc --enable-debug --enable-debug-libs --disable-optimizations
make