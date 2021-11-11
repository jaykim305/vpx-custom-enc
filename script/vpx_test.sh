#!/bin/bash

../build/vpxenc ~/Hearthstone.yuv -o ../output/live_vp9.webm \
                -v \
                --codec=vp9 \
                --i420 \
                -w 1920 \
                -h 1080 \
                --end-usage=cbr \
                --target-bitrate=4000 \
                --fps=60000/1000 \
                --kf-max-dist=90 \
                --kf-min-dist=0 \
                --passes=1 \
                --pass=1 \
                --rt \
                --cpu-used=6 \
                --tile-columns=4 \
                --frame-parallel=1 \
                --threads=8 \
                --static-thresh=0 \
                --max-intra-rate=300 \
                --lag-in-frames=0 \
                --min-q=4 \
                --max-q=48 \
                --row-mt=1 \
                --error-resilient=1