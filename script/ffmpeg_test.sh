#!/bin/bash

content="$1"

#to make it CBR the order should be minrate==maxrate==bitrate

ffmpeg -y -loglevel info -f rawvideo -pix_fmt yuv420p -s 1920x1080 -framerate 60 -i ~/${content}.yuv \
                -keyint_min 0 \
                -s 1920x1080 \
                -r 60 \
                -g 90 \
                -pass 1 \
                -quality realtime \
                -speed 6 \
                -threads 8 \
                -row-mt 1 \
                -tile-columns 4 \
                -frame-parallel 1 \
                -static-thresh 0 \
                -max-intra-rate 300 \
                -lag-in-frames 0 \
                -qmin 4 \
                -qmax 48 \
                -minrate 4000k \
                -maxrate 4000k \
                -b:v 4000k \
                -c:v libvpx-vp9 \
                -error-resilient 1 \
                ../output/ffmpeg_live_vp9_${content}.webm