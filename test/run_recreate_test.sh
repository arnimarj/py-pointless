#!/bin/sh

for i in `(cd test_files; find | grep map)`; do
	echo $i;
	./a.out --re-create ./test_files/$i test.map;
	./a.out --dump-file ./test_files/$i > output_a.txt;
	./a.out --dump-file test.map > output_b.txt;
	diff --brief output_a.txt output_b.txt;
done
