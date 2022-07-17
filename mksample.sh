#!/bin/sh
OUTPUT=sample.bin
SAMPLE_DIR=samples
MIN_RAND=0
MAX_RAND=255
SRC_DIR=$(dirname "$OUTPUT")

rm "${OUTPUT}"

for f in ${SAMPLE_DIR}/*.jpg; do
	garbage_sz=$((${MIN_RAND} + ${RANDOM} % ${MAX_RAND}))
	dd if=/dev/urandom bs=${garbage_sz} count=1 >> "${OUTPUT}"
	cat "$f" >> "${OUTPUT}"
done

final_garbage_sz=$((${MIN_RAND} + ${RANDOM} % ${MAX_RAND}))
dd if=/dev/urandom bs=${final_garbage_sz} count=1 >> "${OUTPUT}"
