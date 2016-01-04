#!/bin/bash

pwd=$PWD
cd "$(dirname "$0")"
gp_dir=../../gameplay/src

for path in $gp_dir/*.bak; do
	file=$(basename $path)
	file=${file%.bak}
	echo "copying $gp_dir/$file to ./$file"
	cp $gp_dir/$file ./$file
done

cd $PWD
