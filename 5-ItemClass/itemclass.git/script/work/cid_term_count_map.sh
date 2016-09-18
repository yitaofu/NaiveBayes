#!/bin/bash
awk '{
	FLAG = $2;
	#
	if(FLAG == "WORDSEG") {
		id = $1;
		cid4 = $3;
		#
		for(i = 4; i <= NF; i ++) {
			term = toupper($i);
			record[term] += 1;
		}
		#
		for(term in record) {
			print cid4"\t"term"\t"record[term]"\tCID-TERM";
		}
		#
		delete record;
	} else if(FLAG == "RATIO") {
		cid4 = $1;
		cid_size = $3;
		total_count = $4;
		#
		print cid4"\t"cid_size"\t"total_count"\tCID-SIZE";
	}
}'
