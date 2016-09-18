//ic_core.h
#ifndef _IC_CORE_H
#define _IC_CORE_H
//
#include "ic_mhash.h"
#include "ic_mem.h"
#include "cJSON.h"
////////////
#define DICT_PATH_MAX_LEN 256
/////////////////////////////////////////////////////////////
//cid_cell
struct cid_cell {
	char cid4[32];
	char cid1[32];
	char cid4_name[128];
	char cid1_name[128];
};

//tag_cell
struct tag_cell {
	char cid1[32];
	char cid4[32];
	char tag_id[32];
	char tag_name[128];
};

//dat_cell
struct dat_cell {
	char cid4[32];
	double score;
};

//ic_conf
struct ic_conf {
	char word_seg_basic[4096];
	char word_seg_split[32];
	char word_exp_split[32];
	char cid_name_def[32];
	size_t tag_num_max;
	char tag_other_cid4[32];
	//
	char bayes_dict_path[DICT_PATH_MAX_LEN];
	char ratio_dict_path[DICT_PATH_MAX_LEN];
	char cid_dict_path[DICT_PATH_MAX_LEN];
	char tag_dict_path[DICT_PATH_MAX_LEN];
	char tag_name_path[DICT_PATH_MAX_LEN];
};

//main
struct ic_core_main {
	//conf
	struct ic_conf * conf;
	/////dict
	struct ic_mhash * mh_bayes_dict;
	struct ic_mhash * mh_ratio_dict;
	struct ic_mhash * mh_cid_dict;
	struct ic_mhash * mh_tag_dict;
	//
	cJSON * tag_list;
	struct tag_cell * tag_other;
	//////////////////////////////////////////////////////
	struct dat_cell * dat_list;
	size_t dat_size;
	//func
	struct ic_core_result * (*read_func)(struct ic_core_main *, char *);
	struct ic_mem_list * (*seg_func)(struct ic_conf *, char *);
	int (*show_func)(struct ic_core_main *, struct ic_core_result *);
};

/////////////////////////////////////////////////////
//result
struct ic_core_result {
	//read buffer
	char * text;
	char * shopid;
	char * id;
	//word_seg
	char * text_trunc;
	struct ic_mem_list * term_list;
	struct ic_mhash_node * term_count;
	size_t term_uniq_size;
	//calc
	struct ic_mhash * mh_cid_weight;
	double def_score;
	//
	struct dat_cell * res_data;
	size_t res_size;
	////////////////////////
	char * output;
};

////////////////////////////////////////////////////////////////////
struct ic_mem_list * word_seg(struct ic_conf * conf, char * text);
struct ic_mem_list * word_explode(struct ic_conf * conf, char * text);
//
struct ic_conf * ic_conf_build(char * conf_file);
void ic_conf_free(struct ic_conf * conf);
void ic_conf_view(struct ic_conf * conf, FILE * fp);
//
struct ic_core_main * ic_core_init(char * ic_conf_file);
struct ic_core_result * ic_core_worker(struct ic_core_main * cm, char * buf);
void ic_core_result_free(struct ic_core_result * cr);
void ic_core_main_free(struct ic_core_main * cm);
char * ic_url_response(struct ic_core_main * cm, char * args);
//
struct ic_core_result * read_buf_bat(struct ic_core_main * cm, char * buf);
struct ic_core_result * read_buf_one(struct ic_core_main * cm, char * buf);
int get_top24(struct ic_core_main * cm, struct ic_core_result * cr);
int get_bat(struct ic_core_main * cm, struct ic_core_result * cr);
int get_cid_only(struct ic_core_main * cm, struct ic_core_result * cr);
int get_cid4(struct ic_core_main * cm, struct ic_core_result * cr);
int get_tag(struct ic_core_main * cm, struct ic_core_result * cr);
int get_weight(struct ic_core_main * cm, struct ic_core_result * cr);
//
#endif
