#!/bin/bash

awk 'BEGIN{
	count = 0;
	total = 0;
}{
	if (total % 100 == 0){
		count += 1;
		print $0 > "testRightResult1";
		if (count > 1000){
			exit;
		}
	}
	total += 1;
}' "testRightResult"
