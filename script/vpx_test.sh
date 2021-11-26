#!/bin/bash

input="$1"
# bitrate="$2"

for bitrate in 10000 20000 30000
do
    # ../build/my_vpxenc ${input}.yuv ${input}_${bitrate}k_vpx.webm ${bitrate}
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../libvpx/

    ../build/vpxenc ${input}.yuv -o ${input}_${bitrate}k_vpx.webm\
                    --codec=vp9 \
                    --i420 \
                    -w 3840 \
                    -h 2160 \
                    --end-usage=cbr \
                    --target-bitrate=${bitrate} \
                    --fps=60000/1000 \
                    --kf-max-dist=90 \
                    --kf-min-dist=0 \
                    --passes=1 \
                    --pass=1 \
                    --rt \
                    --cpu-used=8 \
                    --tile-columns=4 \
                    --frame-parallel=1 \
                    --threads=16 \
                    --static-thresh=0 \
                    --max-intra-rate=300 \
                    --lag-in-frames=0 \
                    --min-q=4 \
                    --max-q=48 \
                    --row-mt=1 \
                    --error-resilient=1 \
                    --arnr-maxframes=0 \
                    --arnr-strength=3 \
                    --arnr-type=3 
                    # --psnr \
                    # --limit=100
done