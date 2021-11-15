#!/bin/bash

content="$1"

../build/my_vpxenc ~/${content}.yuv ../output/mylive_vp9_${content}.webm

# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../libvpx/

# ../libvpx/vpxenc ~/${content}.yuv -o ../output/mylive_vp9_${content}.webm \
#                 --codec=vp9 \
#                 --i420 \
#                 -w 3840 \
#                 -h 2160 \
#                 --end-usage=cbr \
#                 --target-bitrate=8000 \
#                 --fps=60000/1000 \
#                 --kf-max-dist=90 \
#                 --kf-min-dist=0 \
#                 --passes=1 \
#                 --pass=1 \
#                 --rt \
#                 --cpu-used=6 \
#                 --tile-columns=4 \
#                 --frame-parallel=1 \
#                 --threads=16 \
#                 --static-thresh=0 \
#                 --max-intra-rate=300 \
#                 --lag-in-frames=0 \
#                 --min-q=4 \
#                 --max-q=48 \
#                 --row-mt=1 \
#                 --error-resilient=1 \