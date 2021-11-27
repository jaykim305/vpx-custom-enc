#!/bin/bash

input="$1"
type=sr
# bitrate="$2"

#to make it CBR the order should be minrate==maxrate==bitrate

for bitrate in 10000 20000 30000
do
    ffmpeg -y -loglevel info -f rawvideo -pix_fmt yuv420p -s 3840x2160 -framerate 60 -i ${input}/${type}.yuv \
                    -keyint_min 0 \
                    -s 3840x2160 \
                    -r 60 \
                    -g 90 \
                    -pass 1 \
                    -quality realtime \
                    -speed 8 \
                    -threads 16 \
                    -row-mt 1 \
                    -tile-columns 4 \
                    -frame-parallel 1 \
                    -static-thresh 0 \
                    -max-intra-rate 300 \
                    -lag-in-frames 0 \
                    -qmin 4 \
                    -qmax 48 \
                    -minrate ${bitrate}k \
                    -maxrate ${bitrate}k \
                    -b:v ${bitrate}k \
                    -c:v libvpx-vp9 \
                    -error-resilient 1 \
                    ${input}/${type}_${bitrate}k_ffmpeg.webm
done