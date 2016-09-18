#!/bin/bash

awk 'BEGIN{
	totalWord = 0;
	# 获得类的字典
	while (getline < "ClassDict"){
		split($0, classArry, " ");
		class = classArry[1];
		classNum = classArry[2];
		classDict[class] = classNum;
		totalWord += classNum;
	}

	# 获得特征的字典
	while (getline < "FeatDict"){
		featDict[$0] = 1;
	}
	
	FS = "\t";
}{
	if (FILENAME == "trainFile"){
		# 记录类与特征的数量
		split($4, featArry, " ");
		class = $3;

		for (i = 1; i <= length(featArry); i++){
			feat = featArry[i];
			if ( !featDict[feat] ){
				continue;
			}
			wordCount++;
			if ( (class, feat) in featclassDict ){
				featclassDict[class, feat] += 1;
			}else{
				featclassDict[class, feat] = 1;
			}
		}
	}
}END{
	featNum = length(featDict);
	countCond = 0;
	# 输出类的先验概率 | 输出类与未登录特征的条件概率
	for (class in classDict){
		# 先验概率
		classProb = classDict[class] / totalWord;
		classProb = log(classProb);
		print class " " classProb > "ClassProb";

		# 未登录
		Prob = 1 / (classDict[class]+featNum);
		Prob = log(Prob);
		print class " " "NULL" " " Prob > "CondProb";
		countCond++;
	}

	# 输出类与特征的条件概率
	for (str in featclassDict){
		split(str, Arry, SUBSEP);
		class = Arry[1];
		feat = Arry[2];
		featclassProb = (featclassDict[class, feat]+1) / (classDict[class]+featNum);
		featclassProb = log(featclassProb);
		print class " " feat " " featclassProb > "CondProb";
		countCond++;
	}
	print "条件概率的数量：" countCond;
	
}' "ClassDict" "FeatDict" "trainFile"
