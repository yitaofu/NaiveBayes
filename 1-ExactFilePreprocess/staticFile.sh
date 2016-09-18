#!/bin/bash

cat "part-00000" | awk -F "\t" '{
	cid = $3;
	if ( !cidDict[cid] ){
		cidDict[cid] = 1;
	}else{
		cidDict[cid] += 1;
	}
}END{
	total = 0;
	for (cid in cidDict){
		num = cidDict[cid];
		total += num;
		if (num <= 10){
			cidNum["0-10"] += num;
		}else if (num > 10 && num <= 100){
			cidNum["10-100"] += num;
		}else if (num > 100 && num <= 110){
			cidNum["100-110"] += num;
			print cid "\t" num > "newCid";
		}else if (num > 110 && num <= 120){
			cidNum["110-120"] += num;
			print cid "\t" num > "newCid";
		}else if (num > 120 && num <= 1000){
			cidNum["120-1000"] += num;
		}else if (num > 1000 && num <= 2000){
			cidNum["1000-2000"] += num;
		}else if (num > 2000 && num <= 3000){
			cidNum["2000-3000"] += num;
		}else if (num > 3000 && num <= 4000){
			cidNum["3000-4000"] += num;
		}else if (num > 4000 && num <= 5000){
			cidNum["4000-5000"] += num;
		}else{
			cidNum["5000-*"] += num;
		}
	}
	print "total is : " total;
	print "0-10 " cidNum["0-10"];
	print "10-100 " cidNum["10-100"];
	print "100-110 " cidNum["100-110"];
	print "110-120 " cidNum["110-120"];
	print "120-1000 " cidNum["120-1000"];
	print "1000-2000 " cidNum["1000-2000"];
	print "2000-3000 " cidNum["2000-3000"];
	print "3000-4000 " cidNum["3000-4000"];
	print "4000-5000 " cidNum["4000-5000"];
	print "5000-* " cidNum["5000-*"];
}'
