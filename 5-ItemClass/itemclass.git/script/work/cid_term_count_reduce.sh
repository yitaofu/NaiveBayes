#!/bin/bash
awk '{
	if(cid4 != $1) {
		if(cid4 != "" && cid_size > 0 && total_count > 0) {
			for(term in term_list) {
				print cid4"\t"term"\t"term_list[term]"\t"cid_size"\t"total_count;
			}
		}
		#
		cid4 = $1;
		cid_size = 0;
		total_count = 0;
		#
		delete term_list;
	}
	#
	if($NF == "CID-TERM") {
		term = $2;
		term_list[term] += 1;
	} else if($NF == "CID-SIZE") {
		cid_size = $2;
		total_count = $3;
	}
}
END {
	if(cid4 != "" && cid_size > 0 && total_count > 0) {
		for(term in term_list) {
			print cid4"\t"term"\t"term_list[term]"\t"cid_size"\t"total_count;
		}
	}
}'
