#!/bin/bash
awk -F"\t" '{
	cid_count[$1] += 1;
}
END {
	for(cid in cid_count) {
		print cid"\t"cid_count[cid];
	}
}'
