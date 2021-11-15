#!/bin/bash

content="$1"

#ffmpeg -f rawvideo -pix_fmt yuv420p -s 1920x1080 -r 60 -i ../Hearthstone.yuv -i ./output/ffmpeg_live_vp9.webm -lavfi psnr="stats_file=psnr.log" -f null -
#ffmpeg -f rawvideo -pix_fmt yuv420p -s 1920x1080 -r 60 -i ../Hearthstone.yuv -i ./output/live_vp9.webm -lavfi psnr="stats_file=psnr.log" -f null -
ffmpeg -i ../output/lossless_${content}.webm -i ../output/mylive_vp9_${content}.webm -lavfi psnr="stats_file=psnr.log" -f null -
ffmpeg -i ../output/lossless_${content}.webm -i ../output/ffmpeg_live_vp9_${content}.webm -lavfi psnr="stats_file=psnr.log" -f null -