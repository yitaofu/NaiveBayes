#!/bin/bash
awk -F"\t" '{
	id = $1;
	cid = $3;
	terms = $4;
	#
	print cid"\t"id;
}'
