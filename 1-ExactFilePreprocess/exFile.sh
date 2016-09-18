#!/bin/bash

awk -F "\t" 'BEGIN{
	while (getline < "newCid"){
		cid = $1;
		num = $2;
		trainNum = int(num*0.8);
		testNum = num - trainNum;
		cidNum[cid] = trainNum;
		fileNum[cid] = 0;
	}
}{
	if (FILENAME == "part-00000"){
		cid = $3;
		if (cid in cidNum){
			fileNum[cid] += 1;
			if (fileNum[cid] < cidNum[cid]){
				print $0 > "trainFile";
			}else{
				print $0 > "testFile";
			}
		}
	}
}' "newCid" "part-00000"
