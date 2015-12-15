#!/bin/bash

pwd=$PWD
cd "$(dirname "$0")"
gp_dir=../../gameplay/src

for path in ./*; do
	if [[ $path == *.sh ]]; then continue; fi
	file=$(basename $path)
	echo $file
	if [[ -f $gp_dir/$file.bak ]]; then
		echo "  original already backed up"
	else
		echo "  backing up original"
		cp $gp_dir/$file $gp_dir/$file.bak
	fi
	echo "  copying $path to $gp_dir/$file"
	cp $path $gp_dir/$file
done

cd $pwd
