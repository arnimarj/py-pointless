#!/bin/sh

P=/home/arni/pointless_test_files
V="valgrind -q"
#V=

for i in `(cd $P; find | grep map)`; do
	echo $i;
	$V ./new.out --re-create-32 $P/$i test_32.map;
	$V ./new.out --re-create-64 $P/$i test_64.map;
	$V ./new.out --dump-file $P/$i > output_a.txt;
	$V ./new.out --dump-file test_32.map > output_b_32.txt;
	$V ./new.out --dump-file test_64.map > output_b_64.txt;
	diff --brief output_a.txt output_b_32.txt;
	diff --brief output_a.txt output_b_64.txt;
	#./new.out --measure-32-64-bit-difference $P/$i;
done
