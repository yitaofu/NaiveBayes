#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <ctime>

using namespace std;

ifstream fpin_test;
ofstream fpout_test;
pthread_mutex_t pre_mutex;

map<string, double> map_classProb;
map<pair<string, string>, double> map_classfeatProb;
map<string, int> map_stopwords;

int count = 0;
vector<string> vc_predictclass;

// 加载类的先验概率和条件概率
void LoadDict(string className, string classfeatName, string stopwordsName){
	ifstream fpin_class(className.c_str());
	ifstream fpin_classfeat(classfeatName.c_str());
	ifstream fpin_stopwords(stopwordsName.c_str());
	string strline;
	string classN, featN, probS;  // 获得文件中的类名，特征名和相应概率
	double probD;  // 将字符串概率转化为浮点数

	// 获得类的先验概率
	while (getline(fpin_class, strline)){
		stringstream ss(strline);
		ss >> classN;
		ss >> probS;
		probD = atof(probS.c_str());

		map_classProb.insert(pair<string, double>(classN, probD));
	}
	cout << "类的先验概率加载完毕！" << endl;

	// 获得类与特征的条件概率
	pair<string, string> classfeat;  // 类与特征的联合
	while (getline(fpin_classfeat, strline)){
		stringstream ss(strline);
		ss >> classN;
		ss >> featN;
		ss >> probS;
		probD = atof(probS.c_str());

		classfeat = make_pair(classN, featN);
		
		map_classfeatProb.insert(pair<pair<string, string>, double>(classfeat, probD));
	}
	cout << "类与特征的条件概率加载完毕！" << endl;

	// 获得停用词表
	while (getline(fpin_stopwords, strline)){
		if (map_stopwords.find(strline) == map_stopwords.end()){
			map_stopwords.insert(pair<string, int>(strline, 1));
		}
	}
	cout << "停用词表加载完毕！" << endl;

	fpin_class.close();
	fpin_classfeat.close();
	fpin_stopwords.close();
}

// 预测结果(某一类)
string PredictClass(vector<string> vc_feat){
	double maxProb;  // 最大概率
	string maxClass;  // 最大概率对应的类
	double classProb;  // 类的先验概率
	double calProb;  // 计算的概率和
	string classN, featN;  // 类和特征名
	pair<string, string> classfeat;
	map<string, double>::iterator itmap_class;
	map<pair<string, string>, double>::iterator itmap_classfeat;
	int i;

	// 进行预测
	maxProb = -100000000;
	// 循环所有类
	for (itmap_class = map_classProb.begin(); itmap_class != map_classProb.end(); itmap_class++){
		calProb = 0.0;  // 概率初始化
		classN = itmap_class->first;
		classProb = itmap_class->second;

		// 条件概率的加和
		for (i = 0; i < (int)vc_feat.size(); i++){
			featN = vc_feat[i];
			classfeat = make_pair(classN, featN);  // 类与特征联合

			itmap_classfeat = map_classfeatProb.find(classfeat);
			if ( itmap_classfeat != map_classfeatProb.end() ){  // 找到该类与特征
				calProb += itmap_classfeat->second;
			}else{  // 没有找到该类与特征
				classfeat = make_pair(classN, "NULL");  // 类与未登录特征联合
				itmap_classfeat = map_classfeatProb.find(classfeat);
				calProb += itmap_classfeat->second;
			}
		}

		calProb += classProb;  // 加上类的先验概率
		if (calProb > maxProb){
			maxProb = calProb;
			maxClass = classN;
		}
	}

	return maxClass;
}

// 预测模型
void *PredictModel(void *dat){
        string strline;
	vector<string> vc_feat;  // 存放特征
	string feat;
	string predictClass;  // 预测的类别
	int vc_local;

	cout << "进入线程" << pthread_self() << endl;
        while (1){
                pthread_mutex_lock(&pre_mutex);
                if (!getline(fpin_test, strline)){
			cout << "线程" << pthread_self() << "结束" << endl;
			pthread_mutex_unlock(&pre_mutex);
                        break;
                }
		count++;
		if (count % 100 == 0){
			cout << "已经完成" << count << "句!" << endl;
		}
		vc_local = (int)vc_predictclass.size();
		vc_predictclass.push_back("temp");
                pthread_mutex_unlock(&pre_mutex);

		// 筛选特征
		stringstream ss(strline);
		vc_feat.clear();
		while ( ss >> feat ){
			if ( map_stopwords.find(feat) == map_stopwords.end() ){
				vc_feat.push_back(feat);
			}
		}
		
		predictClass = PredictClass(vc_feat);  // 分类结果
//		string tempStr = strline + " | " + predictClass;
		string tempStr = predictClass;
		vc_predictclass[vc_local] = tempStr;
        }

        return 0;
}
// 创建线程
void CreateMultipthread(string testName, int thread_num){
	pthread_t *pid_list = 0;
	int i;

	pthread_mutex_init(&pre_mutex, NULL);  // 互斥锁初始化
	// 申请thread_num个线程
	if ( (pid_list = (pthread_t *)malloc(sizeof(pthread_t)*thread_num)) == NULL ){
		cout << "pid_list malloc error: " << sizeof(pthread_t)*thread_num << endl;
		exit(1);
	}

	fpin_test.open(testName.c_str());
	string resultName = "ResultPredict/" + testName + "Predict";
	fpout_test.open(resultName.c_str());

	// 创建线程
	cout << "创建线程！" << endl;
	for (i = 0; i < thread_num; i++){
		pthread_create(&(pid_list[i]), NULL, PredictModel, NULL);
	}
	for (i = 0; i < thread_num; i++){
		pthread_join(pid_list[i], NULL);
	}
	pthread_mutex_destroy(&pre_mutex);  // 摧毁线程

	free(pid_list);

	for (int i = 0; i < (int)vc_predictclass.size(); i++){
		fpout_test << vc_predictclass[i] << endl;
	}
	fpin_test.close();
	fpout_test.close();
}

int main(int argc, char *argv[]){
	string testName;
	stringstream ss;
	ss << argv[1];
	testName = ss.str();
	int thread_num = atoi(argv[2]);
	string className = "ClassProb";
	string classfeatName = "CondProb";
	string stopwords = "stopwords.txt";
	clock_t start, finish;
	double totaltime;

	start = clock();
	cout << "testName: " << testName << endl;
	cout << "thread_num: " << thread_num << endl;
	LoadDict(className, classfeatName, stopwords);  // 加载模型
	cout << "模型加载完毕！" << endl; 
	CreateMultipthread(testName, thread_num);  // 创建多线程

	cout << "类的先验概率个数: " << map_classProb.size() << endl;
	cout << "类的条件概率个数: " << map_classfeatProb.size() << endl;
	cout << "停用词表的个数: " << map_stopwords.size() << endl;

	finish = clock();
	totaltime = (double)(finish-start) / CLOCKS_PER_SEC;
	cout << "--------运行总时间为" << totaltime << "秒！------------" << endl;

	return 0;
}

