#!/bin/bash

size=512

for file in ../png/item_photos/*; do
  if [[ $file == *"_small"* ]]; then continue; fi
  outfile=$(echo $file | sed 's/\(\.[^.]*\)$/_small.png/')
  if [[ -f $outfile ]]; then continue; fi
  echo "converting $file to $outfile"
  convert $file -resize ${size}x${size}^ -gravity center -crop ${size}x${size}+0+0 +repage $outfile
done
