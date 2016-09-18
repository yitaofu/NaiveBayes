#!/bin/bash

awk -F "\t" '{
	cid = $3;
	term = $4;
	print term > "testFileName";
	print cid > "testRightResult";
}' "testFile"
