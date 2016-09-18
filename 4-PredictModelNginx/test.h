//test
#ifndef _TEST_H
#define _TEST_H
//
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <utility>

using namespace std;

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
////////////////////////////////////////////////
struct test_data {
	double maxProb;
	string maxCid;
	vector<string> vc_term;
	char * data;
};

/////////////
struct test_data * test_init(char * conf_file);
char *urldecode(char *src);
char * test_work(struct test_data * test_data, char * args);
// 预测模型
void *PredictModel(void *dat);
void CreateMultipthread(vector<string> vc_term, int thread_num);

#endif
