#!/bin/bash

inputDir="$1"

echo "create yuv from png ..."
python ../src/png-to-yuv.py --input_dir ${inputDir}

echo "encode videos using vpxenc (bitrate 10,20,30Mbps)..."
bash vpx_test.sh ${inputDir}

echo "encode videos using ffmpeg (bitrate 10,20,30Mbps)..."
bash ffmpeg_test.sh ${inputDir}

echo "extract frames png frames from videos and measuring qualities ..."
bash measure_quality.sh ${inputDir}