#!/bin/bash

# input="$1"
# bitrate="$2"

for bitrate in 10000 20000 30000
do
    python ../src/measure_quality.py --input_dir ~/dataset-all --type vpx --bitrate ${bitrate}
    python ../src/measure_quality.py --input_dir ~/dataset-all --type ffmpeg --bitrate ${bitrate}
done