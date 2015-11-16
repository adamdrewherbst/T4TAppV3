#!/bin/bash

for i in `seq 1 10`;
do
  cropSize=$((2160/$i))
  offset=$((1080 - $cropSize/2))
  convert drillbit_small.png -crop ${cropSize}x720+${offset}+0\! -resize 256x256\! drill_bit_12_${i}0.png
  cropSize=$((1200/$i))
  offset=$((600 - $cropSize/2))
  convert squaredrill_small.png -crop ${cropSize}x388+${offset}+0\! -resize 256x256\! drill_bit_4_${i}0.png
done
