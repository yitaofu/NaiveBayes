#!/bin/bash

awk -F "\t" 'BEGIN{
	while (getline < "stopwords.txt"){
		if ( !stopwordDict[$0] ){
			stopwordDict[$0] = 1;
		}
	}
}{
	if (FILENAME == "trainFile"){
		split($4, featArry, " ");
		wordCount = 0;
		for (i = 1; i <= length(featArry); i++){
			feat = featArry[i];
			if (feat in stopwordDict){
				continue;
			}
			wordCount++;
			if ( !featDict[feat] ){
				featDict[feat] = 1;
			}else{
				featDict[feat] += 1;
			}	
		}

		class = $3;
		if ( !classDict[class] ){
			classDict[class] = wordCount;
		}else{
			classDict[class] += wordCount;
		}
	}

}END{
	for (class in classDict){
		print class " " classDict[class] > "ClassDict";
	}

	print "特征总长度："length(featDict);
	count = 0;
	for (feat in featDict){
#		if (featDict[feat] >= 5){
			print feat > "FeatDict";
			count++;
#		}
	}
	print "保留特征的长度："count;
}' "stopwords.txt" "trainFile"		
