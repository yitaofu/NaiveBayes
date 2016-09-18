#!/bin/bash
awk 'BEGIN {
	total_count = 0;
}
{
	cid4 = $1;
	count = $2;
	#
	cid4_count[cid4] = count;
	total_count += count;
}
END {
	for(cid4 in cid4_count) {
		print cid4"\tRATIO\t"cid4_count[cid4]"\t"total_count;
	}
}'
