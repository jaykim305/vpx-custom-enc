#!/bin/bash

ffmpeg -f rawvideo -pix_fmt yuv420p -s 1920x1080 -r 60000/1000 -i ~/Hearthstone.yuv \
                -r 60000/1000 \
                -s 1920x1080 \
                -c:v vp9 \
                -lossless 1  \
                ../output/lossless.webm 