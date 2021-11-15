#!/bin/bash

cd ./libvpx
bash ./configure --target=x86_64-linux-gcc --enable-runtime-cpu-detect --enable-shared 
make