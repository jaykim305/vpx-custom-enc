#!/bin/bash

content="$1"
echo Encoding $content

ffmpeg -f rawvideo -pix_fmt yuv420p -s 1920x1080 -r 60000/1000 -i ~/${content}.yuv \
                -r 60000/1000 \
                -s 1920x1080 \
                -c:v vp9 \
                -lossless 1  \
                ../output/lossless_${content}.webm 