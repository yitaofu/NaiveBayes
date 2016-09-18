#!/bin/bash
DATA_REMOTE_DIR="/user/rd/ubs/term_weight";
DATA_HOME="/home/rd/work/term_weight/data";
# ##################################################################
TERM_BAYES_REMOTE=${DATA_REMOTE_DIR}/term_bayes;
TERM_BAYES=${DATA_HOME}/term_bayes.dat;
TERM_FISHER_REMOTE=${DATA_REMOTE_DIR}/term_fisher;
TERM_FISHER=${DATA_HOME}/term_fisher.dat;
CID_RATIO_REMOTE=${DATA_REMOTE_DIR}/cid_ratio;
CID_RATIO=${DATA_HOME}/cid_ratio.dat;
# ##########################################################################################
HADOOP_BIN="/home/rd/share/hadoop/bin/hadoop";
# ###################################################3
DICT_BAYES=${DATA_HOME}/bayes.dict;
DICT_FISHER=${DATA_HOME}/fisher.dict;
DICT_RATIO=${DATA_HOME}/ratio.dict;
DICT_SOLR=${DATA_HOME}/solr.dict;

# ##########################################
TERM_COUNT_MIN_NUM=10;
SOLR_DICT_ZERO_LEVEL=3.0;
CID_DEF="DEF";

##################
# GET DATA
${HADOOP_BIN} fs -get ${TERM_BAYES_REMOTE} ${TERM_BAYES}.REMOTE;
${HADOOP_BIN} fs -get ${TERM_FISHER_REMOTE} ${TERM_FISHER}.REMOTE;
${HADOOP_BIN} fs -get ${CID_RATIO_REMOTE} ${CID_RATIO}.REMOTE;
#
cat ${TERM_BAYES}.REMOTE/part-* >${TERM_BAYES};
cat ${TERM_FISHER}.REMOTE/part-* >${TERM_FISHER};
cat ${CID_RATIO}.REMOTE/part-* >${CID_RATIO};
#
rm -rf ${TERM_BAYES}.REMOTE ${TERM_FISHER}.REMOTE ${CID_RATIO}.REMOTE;

#######################
# BAYES
cat ${TERM_BAYES} | awk '{
	term = $1;
	cid4 = $2;
	count = $3;
	cid_size = $4;
	total_count = $5;
	prob = $6;
	term_total_count = $7;
	weight = $8;
	weight_log = $9;
	#
	print term"\t"cid4"\t"weight_log;
}'>${DICT_BAYES};

# FISHER
cat ${TERM_FISHER} | awk '{
	term = $1;
	cid4 = $2;
	count = $3;
	cid_size = $4;
	total_count = $5;
	prob = $6;
	term_total_count = $7;
	weight = $8;
	weight_log = $9;
	#
	print term"\t"cid4"\t"weight_log;
}'>${DICT_FISHER};

# RATIO
cat ${CID_RATIO} | awk '{
	print $1"\t"$3"\t"$4;
}'>${DICT_RATIO};

# SOLR
cat ${TERM_BAYES} | awk -v TERM_COUNT_MIN_NUM=${TERM_COUNT_MIN_NUM} '{
	term = $1;
	cid4 = $2;
	count = $3;
	cid_size = $4;
	total_count = $5;
	prob = $6;
	term_total_count = $7;
	weight = $8;
	weight_log = $9;
	#
	if(cid4 != CID_DEF && term_total_count < TERM_COUNT_MIN_NUM) {
		print term"\t"cid4"\t"weight_log;
	}
}'>${DICT_SOLR};
