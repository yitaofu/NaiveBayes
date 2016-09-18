//test
#include "test.h"
#include <time.h>

map<string, double> map_cidProb;
map<pair<string, string>, double> map_cidtermProb;
map<string, int> map_stopwords;
map<string, double>::iterator itmap_cid;

pthread_mutex_t pre_mutex;
struct test_data te_dat;

// 编码，解码------------------------------------------
static char hex2dec(char ch){
	if (ch >= 'A' && ch <= 'Z'){
		ch = (ch - 'A') + 'a';
	}
	return (ch >= '0' && ch <= '9') ? (ch - '0') : (ch -'a' + 10);
}

static char dec2hex(char ch){
	static char hex[] = "0123456789ABCDEF";
	return hex[ch & 15];
}

// urlencode 编码
char *urlencode(char *src){
	char *psrc = src;
	char *buf;
	char *pbuf;
	int len = strlen(src) * 3 + 1;
	buf = (char *)malloc(len);
	// errdo(buf == NULL, src);
	memset(buf, 0, len);

	pbuf = buf;
        while (*psrc){
                if ((*psrc >= '0' && *psrc <= '9') || (*psrc >= 'a' && *psrc <= 'z') || (*psrc >= 'A' && *psrc <= 'Z')){
                        *pbuf = *psrc;
                }else if (*psrc == '-' || *psrc == '_' || *psrc == '.' || *psrc == '~'){
                        *pbuf = *psrc;
                }else if (*psrc == ' '){
                        *pbuf = '+';
                }else{
                        pbuf[0] = '%';
                        pbuf[1] = dec2hex(*psrc >> 4);
                        pbuf[2] = dec2hex(*psrc & 15);
                        pbuf += 2;
                }

                psrc++;
                pbuf++;
        }

        *pbuf = '\0';
        return buf;
}

// urldecode 解码
char *urldecode(char *src){
	char *psrc = src;
        char *buf;
        char *pbuf;
        int len = strlen(src) + 1;
        buf = (char *)malloc(len);
	// errdo(buf == NULL, src);
	memset(buf, 0, len);

	pbuf = buf;
        while (*psrc){
                if (*psrc == '%'){
                        if (psrc[1] && psrc[2]){
                                *pbuf = hex2dec(psrc[1]) << 4 | hex2dec(psrc[2]);
                                psrc += 2;
                        }
                }else if (*psrc == '+'){
                        *pbuf = ' ';
                }else{
                        *pbuf = *psrc;
                }

                psrc++;
                pbuf++;
        }

        *pbuf = '\0';
        return buf;
}
// ----------------------------------------------------------

//初始化
struct test_data * test_init(char * conf_file) {

	struct test_data *test_data = 0;
	//
	if((test_data = (struct test_data *)malloc(sizeof(struct test_data))) == NULL || (test_data->data = (char *)malloc(strlen(conf_file) + 1)) == NULL) {
		fprintf(stderr, "%s: %d: %s: ERROR: malloc error\n", __FILE__, __LINE__, __FUNCTION__);
		exit(0);
	}
	//////////////////////////////////////////////////////////////////////////////////////
	string CidFileName = "/data/fuyitao/NaiveBayes/4-PredictModelNginx/NbModel/ClassProb";
	string CidTermFileName = "/data/fuyitao/NaiveBayes/4-PredictModelNginx/NbModel/CondProb";
	string StopwordFileName = "/data/fuyitao/NaiveBayes/4-PredictModelNginx/NbModel/stopwords.txt";
	
	ifstream fpin_cid(CidFileName.c_str());
	ifstream fpin_cidterm(CidTermFileName.c_str());
	ifstream fpin_stopword(StopwordFileName.c_str());
	string strline;
	string cidN, termN, probS;  // 获得文件中的类名，特征名和相应概率
	double probD;  // 将字符串概率转化为浮点数

	// 获得先验概率
	while (getline(fpin_cid, strline)){
		stringstream ss(strline);
		ss >> cidN;
		ss >> probS;
		probD = atof(probS.c_str());

		map_cidProb.insert(pair<string, double>(cidN, probD));
	}
	cout << "类的先验概率加载完毕！" << endl;

	// 获得类与特征的条件概率
	pair<string, string> cidterm;  // 类与特征的联合
	while (getline(fpin_cidterm, strline)){
		stringstream ss(strline);
		ss >> cidN;
		ss >> termN;
		ss >> probS;
		probD = atof(probS.c_str());

		cidterm = make_pair(cidN, termN);

		map_cidtermProb.insert(pair<pair<string, string>, double>(cidterm, probD));
	}
	cout << "类与特征的条件概率加载完毕！" << endl;

	// 获得停用词表
	while (getline(fpin_stopword, strline)){
		if (map_stopwords.find(strline) == map_stopwords.end()){
			map_stopwords.insert(pair<string, int>(strline, 1));
		}
	}
	cout << "停用词表加载完毕！" << endl;

	fpin_cid.close();
	fpin_cidterm.close();
	fpin_stopword.close();

	////////////////////////////////////////////////////////////////////////////////////////
	string strData = "";
	int cidLen = (int)map_cidProb.size();
	int cidtermLen = (int)map_cidtermProb.size();
	int stopwordLen = (int)map_stopwords.size();
	stringstream ss1, ss2, ss3;

	ss1 << cidLen;
	strData = "先验概率总长度：" + ss1.str();
	ss2 << cidtermLen;
	strData += " | 条件概率总长度：" + ss2.str();
	ss3 << stopwordLen;
	strData += " | 停用词总长度：" + ss3.str();

	test_data->data = (char *)strData.c_str();

	return test_data;
}

// 预测模型
void *PredictModel(void *dat){
	struct test_data *te_dat = (struct test_data *)dat;
//	double maxProb;  // 最大概率
//	string maxCid = "";  // 最大概率对应的类
	double cidProb;  // 类的先验概率
	double calProb;  // 计算的概率和
	string cidN, termN;  // 类和特征名
	pair<string, string> cidterm;
	map<pair<string, string>, double>::iterator itmap_cidterm;
	int i;

	if ( te_dat->vc_term.empty() ){
		te_dat->maxCid = "";
		return 0;
	}

	// 进行预测
//	maxProb = -100000;
	// 循环所有类
	while (1){
		pthread_mutex_lock(&pre_mutex);
		if ( itmap_cid == map_cidProb.end() ){
			pthread_mutex_unlock(&pre_mutex);
			break;
		}
		calProb = 0.0;  // 概率初始化
		cidN = itmap_cid->first;
		cidProb = itmap_cid->second;
		itmap_cid++;
		pthread_mutex_unlock(&pre_mutex);

		// 条件概率的加和
		for (i = 0; i < (int)te_dat->vc_term.size(); i++){
			termN = te_dat->vc_term[i];
			cidterm = make_pair(cidN, termN);  // 类与特征的联合

			itmap_cidterm = map_cidtermProb.find(cidterm);
			if ( itmap_cidterm != map_cidtermProb.end() ){  // 找到该类与特征
				calProb += itmap_cidterm->second;
			}else{  // 没有找到该类与特征
				cidterm = make_pair(cidN, "NULL");  // 类与未登录特征联合
				itmap_cidterm = map_cidtermProb.find(cidterm);
				calProb += itmap_cidterm->second;
			}
		}

		calProb += cidProb;  // 加上类的先验概率
		pthread_mutex_lock(&pre_mutex);
		if (calProb > te_dat->maxProb){
			te_dat->maxProb = calProb;
			te_dat->maxCid = cidN;
		}
		pthread_mutex_unlock(&pre_mutex);
	}
	return 0;	
}

// 创建线程
void CreateMultipthread(vector<string> vc_term, int thread_num){
	pthread_t *pid_list = 0;
	int i;

	pthread_mutex_init(&pre_mutex, NULL);  // 互斥锁初始化
	// 申请thread_num个线程
	if ( (pid_list = (pthread_t *)malloc(sizeof(pthread_t)*thread_num)) == NULL ){
		cout << "pid_list malloc error" << endl;
		exit(1);
	}

	// 数据初始化
	te_dat.maxProb = -1000000;
	te_dat.vc_term.assign(vc_term.begin(), vc_term.end());
	itmap_cid = map_cidProb.begin();
	// 创建线程
	for (i = 0; i < thread_num; i++){
		pthread_create(&(pid_list[i]), NULL, PredictModel, &te_dat);
	}
	for (i = 0; i < thread_num; i++){
		pthread_join(pid_list[i], NULL);
	}
	pthread_mutex_destroy(&pre_mutex);  // 摧毁线程

	free(pid_list);
}

//工作函数
char * test_work(struct test_data * test_data, char * args) {
	char * res = 0;
	//
	if((res = (char *)malloc(strlen(test_data->data) + strlen(args) + 64)) == NULL) {
		fprintf(stderr, "%s: %d: %s: ERROR: malloc error\n", __FILE__, __LINE__, __FUNCTION__);
		exit(0);
	}
	//
	//////////////////////////////////////////////////////////////
	// 获得特征
	string term;
	string predictCid;
	vector<string> vc_term;
	char *argsStr = urldecode(args);
	string line = argsStr;
	stringstream ss(line);

	while ( ss >> term ){
		if ( map_stopwords.find(term) == map_stopwords.end() ){
			vc_term.push_back(term);
		}
	}

	int thread_num = 10;
	CreateMultipthread(vc_term, thread_num);	
//	PredictModel(vc_term);  // 分类结果

	res[0] = '\0';
	char *str = (char *)te_dat.maxCid.c_str();
	
	sprintf(res, "%s %s %s %s", test_data->data, argsStr, "的分类结果是: ", str);
	//
	return res;
}
