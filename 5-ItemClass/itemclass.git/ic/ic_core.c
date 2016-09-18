#include "url/ic_url.h"
#include "ic_mem.h"
#include "ic_urlcode.h"
#include "ic_mhash.h"
#include "ic_tools.h"
#include "ic_core.h"
#include "err/errdo.h"
#include <math.h>
#include <unistd.h>
#include "cJSON.h"
////////////
#define MAX_BUF_LEN 2048
#define UTF8_MARK 0x80
//////////////////////////////////////////////////////
//truncate
char * text_truncate(char * text, int trunc_len) {
	char * text_trunc = substr(text, 0, trunc_len + trunc_len);
	unsigned char ch = '\0';
	int loc = 0;
	int count = 0;
	int i = 0;
	//
	vardo(1, "trunc_len: %d", trunc_len);
	//
	for(i = strlen(text_trunc) - 1; i > trunc_len; i --) {
		if(*(text_trunc + i) == ' ' || *(text_trunc + i) == '\n') {
			*(text_trunc + i) = '\0';
			break;
		}
	}
	//UTF8 ENCODE
	count = 0;
	for(i = strlen(text_trunc) - 1; i >= 0; i --) {
		ch = (unsigned char)(text_trunc[i]);
		//
		if(((ch & UTF8_MARK) != 0) && (((ch << 1) & UTF8_MARK) == 0)) {
			count ++;
			continue;
		}
		//
		break;
	}
	//
	loc = i;
	if(loc >= 0) {
		//
		if(count > 0) {
			count += 1;
		}
		//
		for(i = 0; i < 6; i ++) {
			if(((ch << i) & UTF8_MARK) == 0) {
				break;
			}
			count --;
		}
		//
		if(count != 0) {
			text_trunc[loc] = '\0';
		}
	} else {
		text_trunc[0] = '\0';
	}
	//
	return text_trunc;
}

//word_seg
struct ic_mem_list * word_seg(struct ic_conf * conf, char * text) {
	char url[4096];
	char * encode;
	char * cont;
	struct ic_mem_list * list = 0;
	struct ic_mem_list_node * node = 0;
	////////////////////////
	encode = urlencode(text);
	strcpy(url, conf->word_seg_basic);
	strcat(url, encode);
	//
	vardo(10, "%s", url);
	cont = get_url_contents(url);
	vardo(10, "%s", cont);
	//
	if(cont != NULL) {
		list = explode_clean(cont, conf->word_seg_split);
		//
		node = list->head;
		while(node != NULL) {
			node->key = strtoupper(node->key);
			node = node->next;
		}
		//
		free(cont);
	}
	//
	free(encode);
	return list;
}

//word_split
struct ic_mem_list * word_explode(struct ic_conf * conf, char * text) {
	struct ic_mem_list * list = 0;
	struct ic_mem_list_node * node = 0;
	//
	if(text) {
		list = explode_clean(text, conf->word_exp_split);
		//
		node = list->head;
		while(node != NULL) {
			node->key = strtoupper(node->key);
			node = node->next;
		}
	}
	//
	return list;
}

///////////////////////////////////////////////////////////////////////////
//dict_build
////
//bayes_dict_normalize
void bayes_dict_normalize(struct ic_conf * conf, char * term, struct ic_mem_list * list) {
	double * score;
	double * def_score = 0;
	struct ic_mem_list_node * node;
	/////////
	node = list->head;
	while(node) {
		if(strcmp(node->key, conf->cid_name_def) == 0) {
			def_score = (double *)node->value;
			break;
		}
		node = node->next;
	}
	//
	errdo(def_score == NULL, "%s", term);
	//
	node = list->head;
	while(node) {
		if(strcmp(node->key, conf->cid_name_def) != 0) {
			score = (double *)node->value;
			*score -= *def_score;
		}
		node = node->next;
	}
}

//bayes_dict_load
struct ic_mhash * bayes_dict_load(struct ic_conf * conf) {
	FILE * fp;
	char buf[MAX_BUF_LEN];
	//
	char term[MAX_BUF_LEN];
	char cid[32];
	double score;
	////////////////////
	char * term_p;
	char * cid_p;
	double * score_p;
	////////////////////////////////////////////////////////////////////////////
	struct ic_mhash * mh;
	struct ic_mem_list * list;
	//
	struct ic_mhash_node * node;
	size_t i = 0;
	//
	errdo((mh = ic_mhash_init(9999991)) == NULL, "");
	errdo((fp = fopen(conf->bayes_dict_path, "r")) == NULL, conf->bayes_dict_path);
	////////
	while(fgets(buf, MAX_BUF_LEN, fp) != NULL) {
		if(strlen(buf) >= (MAX_BUF_LEN - 1)) {
			vardo(0, "%s", buf);
			continue;
		}
		sscanf(buf, "%s\t%s\t%lf\n", term, cid, &score);
		//
		if((list = (struct ic_mem_list *)ic_mhash_get(mh, term)) == NULL) {
			term_p = (char *)malloc(strlen(term) + 1);
			errdo(term_p == NULL, "%s", term);
			strcpy(term_p, term);
			//////
			list = ic_mem_list_init();
			ic_mhash_put(mh, term_p, list);
		}
		//
		cid_p = (char *)malloc(strlen(cid) + 1);
		score_p = (double *)malloc(sizeof(score));
		////////////
		errdo(cid_p == NULL || score_p == NULL, "");
		///////
		strcpy(cid_p, cid);
		*score_p = score;
		///////////
		ic_mem_list_add(list, cid_p, strlen(cid_p) + 1, score_p, sizeof(double));
		//////////////
	}
	//
	fclose(fp);
	//
	for(i = 0; i < mh->basic_size; i ++) {
		node = mh->head + i;
		while(node != NULL) {
			if(node->key != NULL) {
				bayes_dict_normalize(conf, node->key, node->value);
			}
			node = node->next;
		}
	}
	//
	return mh;
}

//ratio_dict_load
struct ic_mhash * ratio_dict_load(struct ic_conf * conf) {
	FILE * fp;
	char cid[32];
	int count;
	int count_total;
	//
	char * cid_p;
	double * ratio_p;
	//
	struct ic_mhash * mh = ic_mhash_init(10007);
	errdo(mh == NULL, "");
	fp = fopen(conf->ratio_dict_path, "r");
	errdo(fp == NULL, "%s", conf->ratio_dict_path);
	/////////////////////////////
	while(fscanf(fp, "%s\t%d\t%d\n", cid, &count, &count_total) == 3) {
		cid_p = (char *)malloc(strlen(cid) + 1);
		ratio_p = (double *)malloc(sizeof(double));
		errdo(cid_p == NULL || ratio_p == NULL, "");
		//
		strcpy(cid_p, cid);
		*ratio_p = log((double) count / (double) count_total);
		//
		ic_mhash_put(mh, cid_p, ratio_p);
	}
	//
	fclose(fp);
	//
	return mh;
}

//cid_dict_load
struct ic_mhash * cid_dict_load(struct ic_conf * conf) {
	FILE * fp;
	char buf[2048];
	struct ic_mem_list * data_list;
	struct ic_mem_list_node * node;
	//
	char * key;
	struct cid_cell * cell;
	struct ic_mhash * mh = ic_mhash_init(10007);
	errdo(mh == NULL, "");
	fp = fopen(conf->cid_dict_path, "r");
	errdo(fp == NULL, "%s", conf->cid_dict_path);
	//
	while(fgets(buf, 2048, fp) != NULL) {
		data_list = explode_clean(buf, "\t");
		errdo(data_list->size != 4, "");
		cell = (struct cid_cell *)malloc(sizeof(struct cid_cell));
		errdo(cell == NULL, "");
		//
		node = data_list->head;
		strcpy(cell->cid4, node->key);
		node = node->next;
		strcpy(cell->cid1, node->key);
		node = node->next;
		strcpy(cell->cid4_name, node->key);
		node = node->next;
		strcpy(cell->cid1_name, node->key);
		//
		key = (char *)malloc(strlen(cell->cid4) + 1);
		strcpy(key, cell->cid4);
		//
		ic_mhash_put(mh, key, cell);
		//
		ic_mem_list_free_all(data_list);
	}
	fclose(fp);
	//
	return mh;
}

//tag_dict_load
struct ic_mhash * tag_dict_load(struct ic_conf * conf) {
	FILE * fp;
	char cid4[32];
	char cid1[32];
	char tag_id[32];
	char tag_name[128];
	//
	char * key;
	struct tag_cell * cell;
	struct ic_mhash * mh = ic_mhash_init(10007);
	errdo(mh == NULL, "");
	fp = fopen(conf->tag_dict_path, "r");
	errdo(fp == NULL, "%s", conf->tag_dict_path);
	/////////////////////////////
	while(fscanf(fp, "%s\t%s\t%s\t%s\n", cid4, cid1, tag_id, tag_name) == 4) {
		cell = (struct tag_cell *)malloc(sizeof(struct tag_cell));
		errdo(cell == NULL, "");
		strcpy(cell->cid4, cid4);
		strcpy(cell->cid1, cid1);
		strcpy(cell->tag_id, tag_id);
		strcpy(cell->tag_name, tag_name);
		//
		key = (char *)malloc(strlen(cell->cid4) + 1);
		strcpy(key, cell->cid4);
		//
		ic_mhash_put(mh, key, cell);
	}
	fclose(fp);
	//
	return mh;
}

//tag_name_load
cJSON * tag_name_load(struct ic_conf * conf) {
	FILE * fp;
	char tag_id[32];
	char tag_name[128];
	//
	cJSON * tag_list;
	cJSON * node;
	//
	tag_list = cJSON_CreateArray();
	errdo(tag_list == NULL, "");
	//
	fp = fopen(conf->tag_name_path, "r");
	errdo(fp == NULL, "%s", conf->tag_name_path);
	////////////////////////////////////////////////////////////////
	while(fscanf(fp, "%s\t%s\n", tag_id, tag_name) == 2) {
		cJSON_AddItemToArray(tag_list, node = cJSON_CreateObject());
		cJSON_AddStringToObject(node, "tag_id", tag_id);
		cJSON_AddStringToObject(node, "name", tag_name);
	}
	//
	fclose(fp);
	//
	return tag_list;
}
//////////////
//dict_view
////////////////////////////////////////////
//bayes_dict_node_view
void bayes_dict_node_view(char * key, void * value) {
	struct ic_mem_list * list = (struct ic_mem_list *)value;
	struct ic_mem_list_node * node;
	//
	if(list == NULL) {
		return;
	}
	/////////////////
	node = list->head;
	while(node) {
		printf("%s\t%s\t%.6lf\n", key, (char *)node->key, *((double *)node->value));
		node = node->next;
	}
}

//ratio_dict_node_view
void ratio_dict_node_view(char * key, void * value) {
	double * ratio = (double *)value;
	printf("%s\t%.6lf\n", key, *ratio);
}

//cid_dict_node_view
void cid_dict_node_view(char * key, void * value) {
	struct cid_cell * cell = (struct cid_cell *)value;
	printf("%s\t%s\t%s\t%s\n", cell->cid4, cell->cid1, cell->cid4_name, cell->cid1_name);
}

//tag_dict_node_view
void tag_dict_node_view(char * key, void * value) {
	struct tag_cell * cell = (struct tag_cell *)value;
	printf("%s\t%s\t%s\t%s\n", cell->cid4, cell->cid1, cell->tag_id, cell->tag_name);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//cid_score_clone
void * cid_score_clone(char * key, void * value) {
	double * score = (double *)malloc(sizeof(double));
	errdo(score == NULL, "");
	//
	*score = *((double *)value);
	//
	return score;
}

//cid_score_view
void cid_score_view(char * key, void * value) {
	double * score = (double *)value;
	printf("%s\t%.6lf\n", key, *score);
}

//cid_score_compar
int cid_score_compar(const void * nodea, const void * nodeb) {
	struct ic_mhash_node * a = (struct ic_mhash_node *) nodea;
	struct ic_mhash_node * b = (struct ic_mhash_node *) nodeb;
	//
	double sa = *((double *)(a->value));
	double sb = *((double *)(b->value));
	double diff = sb - sa;
	//////////////////////////////////////////////////
	if(diff > 1.0E-8) {
		return 1;
	} else if(diff < -1.0E-8) {
		return -1;
	} else {
		return 0;
	}
}

//dat_cell_compar
int dat_cell_compar(const void * cella, const void * cellb) {
	struct dat_cell * a = (struct dat_cell *)cella;
	struct dat_cell * b = (struct dat_cell *)cellb;
	//
	double sa = a->score;
	double sb = b->score;
	double diff = sb - sa;
	//
	if(diff > 1.0E-8) {
		return 1;
	} else if(diff < -1.0E-8) {
		return -1;
	} else {
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
//prepare
void term_count_view(char * key, void * value) {
	printf("TERM-COUNT: %s\t%d\n", key, *((int *)value));
}

//res_prepare
int res_prepare(struct ic_core_main * cm, struct ic_core_result * cr, struct ic_mem_list * seg_func(struct ic_conf *, char *)) {
	struct ic_mhash * mh;
	struct ic_mem_list_node * node;
	char * term;
	int * count;
	int trunc_len = (seg_func == word_explode) ? 90 : 60;
	//
	cr->text_trunc = text_truncate(cr->text, trunc_len);
	cr->term_list = seg_func(cm->conf, cr->text_trunc);
	//
	mh = ic_mhash_init(37);
	errdo(mh == NULL, "");
	//
	if(cr->term_list == NULL) {
		return -1;
	}
	//
	node = cr->term_list->head;
	while(node) {
		if((count = (int *)ic_mhash_get(mh, (char *)node->key)) == NULL) {
			term = (char *)malloc(strlen(node->key) + 1);
			count = (int *)malloc(sizeof(int));
			errdo(term == NULL || count == NULL, "");
			//
			strcpy(term, node->key);
			*count = 0;
			//
			ic_mhash_put(mh, term, count);
		}
		*count += 1;
		//
		node = node->next;
	}
	//
	cr->term_count = ic_mhash_to_array(mh, &(cr->term_uniq_size));
	ic_mhash_free(mh);
	/*
	size_t i = 0;
	for(i = 0; i < cr->term_uniq_size; i ++) {
		term_count_view(cr->term_count[i].key, cr->term_count[i].value);
	}
	*/
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////
//bayes_calc
int bayes_calc(struct ic_core_main * cm, struct ic_core_result * cr) {
	size_t i = 0;
	struct ic_mem_list * score_list;
	struct ic_mem_list_node * score_node;
	//
	char * cid;
	double * score;
	double * final_score;
	//
	vardo(10, "calc start");
	//
	cr->res_size = cm->dat_size;
	cr->res_data = (struct dat_cell *)malloc(cr->res_size * sizeof(struct dat_cell));
	errdo(cr->res_data == NULL, "");
	memmove(cr->res_data, cm->dat_list, cr->res_size * sizeof(struct dat_cell));
	//
	cr->mh_cid_weight = ic_mhash_init_core(cm->mh_ratio_dict->basic_size, cm->mh_ratio_dict->box_size);
	for(i = 0; i < cr->res_size; i ++) {
		ic_mhash_put(cr->mh_cid_weight, (cr->res_data + i)->cid4, &((cr->res_data + i)->score));
	}
	//cr->mh_cid_weight = ic_mhash_clone(cm->mh_ratio_dict, cid_score_clone);
	cr->def_score = 0.0;
	vardo(10, "cid_weight_clone");
	//
	for(i = 0; i < cr->term_uniq_size; i ++) {
		score_list = ic_mhash_get(cm->mh_bayes_dict, cr->term_count[i].key);
		if(score_list != NULL) {
			score_node = score_list->head;
			while(score_node != NULL) {
				cid = (char *)(score_node->key);
				score = (double *)(score_node->value);
				if(strcmp(cid, cm->conf->cid_name_def) == 0) {
					cr->def_score += *score;
				} else {
					final_score = ic_mhash_get(cr->mh_cid_weight, cid);
					if(final_score == NULL) {
						final_score = (double *)malloc(sizeof(double));
						*final_score = 0.0;
						ic_mhash_put(cr->mh_cid_weight, cid, final_score);
					}
					//
					*final_score += *score;
				}
				////////////
				score_node = score_node->next;
			}
		}
	}
	//
	vardo(10, "add");
	//cr->res_data = ic_mhash_to_array(cr->mh_cid_weight, &(cr->res_size));
	vardo(10, "to_array");
	for(i = 0; i < cr->res_size; i ++) {
		cr->res_data[i].score += cr->def_score;
	}
	vardo(10, "def_score");
	//
	//qsort(cr->res_data, cr->res_size, sizeof(struct ic_mhash_node), cid_score_compar);
	qsort(cr->res_data, cr->res_size, sizeof(struct dat_cell), dat_cell_compar);
	vardo(10, "sort");
	//
	return 0;
}

/////////////////////////////////////////////
//tools
//accu_calc
double accu_calc(struct ic_core_result * cr) {
	double accu = 0.0;
	size_t num = 24;
	size_t i = 0;
	double score_def = cr->res_data[0].score;
	double sum = 1.0;
	//
	if(num > cr->res_size) {
		num = cr->res_size;
	}
	//
	for(i = 1; i < num; i ++) {
		sum += exp(cr->res_data[i].score - score_def);
	}
	//
	accu = 1.0 / sum;
	//
	return accu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//show result
//get_cid_only
int get_cid_only(struct ic_core_main * cm, struct ic_core_result * cr) {
	char * res = 0;
	char * cid4 = 0;
	//
	cid4 = cr->res_data[0].cid4;
	//
	res = (char *)malloc(strlen(cid4) + 1);
	if(res) {
		memmove(res, cid4, strlen(cid4) + 1);
	}
	//
	cr->output = res;
	//
	return 0;
}

//get_cid4
int get_cid4(struct ic_core_main * cm, struct ic_core_result * cr) {
	cJSON * root;
	char * res = 0;
	//
	struct cid_cell * cid_cell = 0;
	char * cid4 = 0;
	char * cid1 = 0;
	char * cid4_name = 0;
	char * cid1_name = 0;
	//
	char def_data[] = "NULL";
	//
	cid4 = cr->res_data[0].cid4;
	//
	cid_cell = ic_mhash_get(cm->mh_cid_dict, cid4);
	if(cid_cell) {
		cid1 = cid_cell->cid1;
		cid4_name = cid_cell->cid4_name;
		cid1_name = cid_cell->cid1_name;
	} else {
		cid1 = def_data;
		cid4_name = def_data;
		cid1_name = def_data;
	}
	//
	root = cJSON_CreateObject();
	if(root) {
		cJSON_AddNumberToObject(root, "status", 0);
		cJSON_AddStringToObject(root, "text_trunc", cr->text_trunc);
		cJSON_AddStringToObject(root, "cid4", cr->res_data[0].cid4);
		cJSON_AddStringToObject(root, "cid4_name", cid4_name);
		cJSON_AddStringToObject(root, "cid1", cid1);
		cJSON_AddStringToObject(root, "cid1_name", cid1_name);
		cJSON_AddNumberToObject(root, "accuracy", accu_calc(cr));
		cJSON_AddNumberToObject(root, "term_uniq_size", cr->term_uniq_size);
		//
		res = cJSON_PrintUnformatted(root);
		//
		cJSON_Delete(root);
	}
	//
	cr->output = res;
	return 0;
}

//get_tag
int get_tag(struct ic_core_main * cm, struct ic_core_result * cr) {
	cJSON * root;
	cJSON * node;
	cJSON * tag_node;
	char * res = 0;
	char * cid4;
	struct tag_cell * cell;
	//
	struct ic_mhash * mh_cell = 0;
	size_t i = 0;
	//
	mh_cell = ic_mhash_init(cm->conf->tag_num_max * 2);
	//
	cid4 = cr->res_data[0].cid4;
	cell = ic_mhash_get(cm->mh_tag_dict, cid4);
	//
	if(mh_cell != NULL && cell != NULL) {
		vardo(0, "cid4: %s, tag_id: %s, tag_name: %s", cid4, cell->tag_id, cell->tag_name);
		//
		if(cm->tag_other) {
			ic_mhash_put(mh_cell, cm->tag_other->tag_name, cm->tag_other);
		}
		//
		root = cJSON_CreateObject();
		if(root) {
			cJSON_AddNumberToObject(root, "status", 0);
			cJSON_AddStringToObject(root, "cid", cid4);
			cJSON_AddItemToObject(root, "default", node = cJSON_CreateObject());
			cJSON_AddStringToObject(node, "tag_id", cell->tag_id);
			cJSON_AddStringToObject(node, "name", cell->tag_name);
			//
			cJSON_AddItemToObject(root, "tag_list", node = cJSON_CreateArray());
			//
			for(i = 0; i < cr->res_size; i ++) {
				cell = ic_mhash_get(cm->mh_tag_dict, cr->res_data[i].cid4);
				if(ic_mhash_get(mh_cell, cell->tag_name) == NULL) {
					cJSON_AddItemToArray(node, tag_node = cJSON_CreateObject());
					cJSON_AddStringToObject(tag_node, "tag_id", cell->tag_id);
					cJSON_AddStringToObject(tag_node, "name", cell->tag_name);
					//
					ic_mhash_put(mh_cell, cell->tag_name, cell);
					if(mh_cell->data_size >= cm->conf->tag_num_max) {
						break;
					}
				}
			}
			//
			if(cm->tag_other) {
				cJSON_AddItemToArray(node, tag_node = cJSON_CreateObject());
				cJSON_AddStringToObject(tag_node, "tag_id", cm->tag_other->tag_id);
				cJSON_AddStringToObject(tag_node, "name", cm->tag_other->tag_name);
			}
			//
			res = cJSON_PrintUnformatted(root);
			//
			cJSON_Delete(root);
		}
		//
		ic_mhash_free(mh_cell);
	} else {
		root = cJSON_CreateObject();
		if(root) {
			cJSON_AddNumberToObject(root, "status", -1);
			res = cJSON_PrintUnformatted(root);
			//
			cJSON_Delete(root);
		}
	}
	//
	cr->output = res;
	return 0;
}

//get_top_24
int get_top24(struct ic_core_main * cm, struct ic_core_result * cr) {
	size_t num = 24;
	size_t i = 0;
	char * res;
	char * loc;
	//
	if(num > cr->res_size) {
		num = cr->res_size;
	}
	//
	if(cr->id != NULL){
		res = (char *)malloc(strlen(cr->text_trunc) + 1 + num * (12 + 20) + 1);
		if(res != NULL) {
			loc = res;
			sprintf(loc, "%s", cr->text_trunc);
			loc = res + strlen(res);
			*loc = '\n';
			loc += 1;
			for(i = 0; i < num; i ++) {
				sprintf(loc, "%s\t%.6lf\n", cr->res_data[i].cid4, cr->res_data[i].score);
				loc = res + strlen(res);
			}
		}
	} else {
		res = (char *)malloc(strlen(cr->text_trunc) + 1);
		if(res != NULL){
			loc = res;
			sprintf(loc, "%s\n", cr->text_trunc);
		}
	}
	//
	cr->output = res;
	return 0;
}

//get_bat
int get_bat(struct ic_core_main * cm, struct ic_core_result * cr) {
	char * cid4;
	char * cid1;
	char * cid4_name;
	char * cid1_name;
	//
	double score;
	char * output;
	int op_len = 0;
	struct cid_cell * cid_cell;
	char def_data[] = "NULL";
	//
	char * id = def_data;
	char * shopid = def_data;
	//
	if(cr->id != NULL) {
		id = cr->id;
	}
	if(cr->shopid != NULL) {
		shopid = cr->shopid;
	}
	//
	cid4 = cr->res_data[0].cid4;
	score = cr->res_data[0].score;
	///////////////////////////////////////////
	cid_cell = ic_mhash_get(cm->mh_cid_dict, cid4);
	//
	op_len = strlen(id) + 1 + strlen(shopid) + 1 + strlen(cid4);
	if(cid_cell) {
		cid1 = cid_cell->cid1;
		cid4_name = cid_cell->cid4_name;
		cid1_name = cid_cell->cid1_name;
	} else {
		cid1 = def_data;
		cid4_name = def_data;
		cid1_name = def_data;
	}
	op_len += (1 + strlen(cid1) + 1 + strlen(cid4_name) + 1 + strlen(cid1_name));
	//
	op_len += (1 + 12);
	//
	op_len += (1 + strlen(cr->text_trunc));
	op_len += 1;
	//
	output = (char *)malloc(op_len);
	errdo(output == NULL, "");
	//
	sprintf(output, "%s\t%s\t%s\t%s\t%s\t%s\t%.6lf\t%s", id, shopid, cid4, cid1, cid4_name, cid1_name, accu_calc(cr), cr->text_trunc);
	//
	cr->output = output;
	return 0;
}

//get_weight
int get_weight(struct ic_core_main * cm, struct ic_core_result * cr) {
	cJSON * root;
	cJSON * node;
	cJSON * term_node;
	//
	char * res = 0;
	char * cid4;
	struct ic_mem_list * score_list = 0;
	struct ic_mem_list * term_score_list = 0;
	struct ic_mem_list_node * score_node = 0;
	char * cid = 0;
	char * term = 0;
	double * term_cid_score;
	size_t i;
	//
	cid4 = cr->res_data[0].cid4;
	term_score_list = ic_mem_list_init();
	//
	for(i = 0; i < cr->term_uniq_size; i ++) {
		score_list = ic_mhash_get(cm->mh_bayes_dict, cr->term_count[i].key);
		term = (char *)malloc(strlen(cr->term_count[i].key) + 1);
		strcpy(term, cr->term_count[i].key);
		if(score_list != NULL) {
			score_node = score_list->head;
			term_cid_score = (double *)malloc(sizeof(double));
			* term_cid_score = 0.0;
			while(score_node != NULL) {
				cid = (char *)(score_node->key);
				if(strcmp(cid, cid4) == 0 || strcmp(cid, "DEF") == 0) {
					* term_cid_score += * (double *)(score_node->value);
				} 
				score_node = score_node->next;
			}
			ic_mem_list_add(term_score_list, term, strlen(term)+1, term_cid_score, 12 + 1);
		}
	}

	if(term_score_list != NULL) {
		vardo(0, "cid4: %s", cid4);
		//
		root = cJSON_CreateObject();
		if(root) {
			cJSON_AddNumberToObject(root, "status", 0);
			cJSON_AddStringToObject(root, "cid", cid4);
			cJSON_AddItemToObject(root, "term_list", node = cJSON_CreateArray());
			//
			score_node = term_score_list->head;
			while(score_node){
				cJSON_AddItemToArray(node, term_node = cJSON_CreateObject());
				cJSON_AddStringToObject(term_node, "term", (char *)score_node->key);
				cJSON_AddNumberToObject(term_node, "term_score", * (double *)score_node->value);
				score_node = score_node->next;
			}
			res = cJSON_PrintUnformatted(root);
			//
			cJSON_Delete(root);
		}
	} else {
		root = cJSON_CreateObject();
		if(root) {
			cJSON_AddNumberToObject(root, "status", -1);
			res = cJSON_PrintUnformatted(root);
			cJSON_Delete(root);
		}
	}
	//
	cr->output = res;
	ic_mem_list_free_all(term_score_list);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//read buf
//read_buf_bat
struct ic_core_result * read_buf_bat(struct ic_core_main * cm, char * buf) {
	struct ic_mem_list * data_list;
	struct ic_core_result * cr = (struct ic_core_result *)malloc(sizeof(struct ic_core_result));
	errdo(cr == NULL, "");
	memset(cr, 0, sizeof(struct ic_core_result));
	//
	data_list = explode_clean(buf, "\t");
	wardo(data_list->size != 3, "%s", buf);
	//
	if(data_list->size == 3) {
		cr->id = (char *)(data_list->head->key);
		cr->shopid = (char *)(data_list->head->next->key);
		cr->text = (char *)(data_list->head->next->next->key);
		//
		vardo(10, "%s, %s, %s", cr->id, cr->shopid, cr->text);
		//
		ic_mem_list_free(data_list);
	} else {
		ic_mem_list_free_all(data_list);
	}
	//
	return cr;
}

//read_buf_one
struct ic_core_result * read_buf_one(struct ic_core_main * cm, char * buf) {
	char * text;
	struct ic_core_result * cr = (struct ic_core_result *)malloc(sizeof(struct ic_core_result));
	errdo(cr == NULL, "");
	memset(cr, 0, sizeof(struct ic_core_result));
	//
	text = (char *)malloc(strlen(buf) + 1);
	strcpy(text, buf);
	trim(text);
	//
	cr->text = text;
	return cr;
}

//////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ic_core_init
//load dict
struct ic_core_main * ic_core_init(char * ic_conf_file) {
	struct ic_mhash_node * mh_array_tmp;
	size_t i = 0;
	struct ic_conf * conf = 0;
	struct ic_core_main * cm = (struct ic_core_main *)malloc(sizeof(struct ic_core_main));
	errdo(cm == NULL, "");
	///////////////////////////////
	conf = ic_conf_build(ic_conf_file);
	//
	cm->mh_bayes_dict = bayes_dict_load(conf);
	cm->mh_ratio_dict = ratio_dict_load(conf);
	cm->mh_cid_dict = cid_dict_load(conf);
	cm->mh_tag_dict = tag_dict_load(conf);
	cm->tag_list = tag_name_load(conf);
	//
	cm->tag_other = (strcmp(conf->tag_other_cid4, "")) ? ic_mhash_get(cm->mh_tag_dict, conf->tag_other_cid4) : NULL;
	//
	mh_array_tmp = ic_mhash_to_array(cm->mh_ratio_dict, &(cm->dat_size));
	cm->dat_list = (struct dat_cell *)malloc(cm->dat_size * sizeof(struct dat_cell));
	errdo(cm->dat_list == NULL, "");
	//
	for(i = 0; i < cm->dat_size; i ++) {
		strcpy((cm->dat_list + i)->cid4, (mh_array_tmp + i)->key);
		(cm->dat_list + i)->score = *((double *)((mh_array_tmp + i)->value));
	}
	//
	free(mh_array_tmp);
	//
	qsort(cm->dat_list, cm->dat_size, sizeof(struct dat_cell), dat_cell_compar);
	//
	cm->conf = conf;
	//
	return cm;
}

//ic_core_worker
//read buffer, create the result
struct ic_core_result * ic_core_worker(struct ic_core_main * cm, char * buf) {
	struct ic_core_result * cr;
	//read buf
	cr = cm->read_func(cm, buf);
	//
	if(cr->text != NULL) {
		//res_prepare
		if(res_prepare(cm, cr, cm->seg_func) == 0) {
			//calc
			bayes_calc(cm, cr);
			//
			cm->show_func(cm, cr);
		}
	}
	//
	return cr;
}

//ic_core_result_free
void ic_core_result_free(struct ic_core_result * cr) {
	size_t i = 0;
	//
	if(cr == NULL) {
		return;
	}
	///////////////////////////////////
	if(cr->id) {
		free(cr->id);
	}
	if(cr->shopid) {
		free(cr->shopid);
	}
	if(cr->text) {
		free(cr->text);
	}
	//
	if(cr->text_trunc) {
		free(cr->text_trunc);
	}
	//
	if(cr->term_list) {
		ic_mem_list_free_all(cr->term_list);
	}
	//
	if(cr->term_count) {
		for(i = 0; i < cr->term_uniq_size; i ++) {
			free((cr->term_count[i]).key);
			free((cr->term_count[i]).value);
		}
		free(cr->term_count);
	}
	//
	if(cr->mh_cid_weight) {
		//ic_mhash_free_all(cr->mh_cid_weight);
		ic_mhash_free(cr->mh_cid_weight);
	}
	//data shared with mh_cid_weight
	if(cr->res_data) {
		free(cr->res_data);
	}
	//
	if(cr->output) {
		free(cr->output);
	}
	//
	free(cr);
}

//ic_core_main_free
void ic_core_main_free(struct ic_core_main * cm) {
	struct ic_mhash_node * node;
	size_t i = 0;
	//
	if(cm == NULL) {
		return;
	}
	//
	if(cm->mh_bayes_dict) {
		for(i = 0; i < cm->mh_bayes_dict->basic_size; i ++) {
			node = cm->mh_bayes_dict->head + i;
			if(node->value != NULL) {
				ic_mem_list_free_all((struct ic_mem_list *)(node->value));
				node->value = 0;
			}
			node = node->next;
			while(node != NULL) {
				if(node->value != NULL) {
					ic_mem_list_free_all((struct ic_mem_list *)(node->value));
					node->value = 0;
				}
				node = node->next;
			}
		}
		//
		ic_mhash_free_all(cm->mh_bayes_dict);
	}
	//
	if(cm->mh_ratio_dict) {
		ic_mhash_free_all(cm->mh_ratio_dict);
	}
	//
	if(cm->mh_cid_dict) {
		ic_mhash_free_all(cm->mh_cid_dict);
	}
	//
	if(cm->mh_tag_dict) {
		ic_mhash_free_all(cm->mh_tag_dict);
	}
	//
	if(cm->tag_list) {
		cJSON_Delete(cm->tag_list);
	}
	//
	if(cm->dat_list) {
		free(cm->dat_list);
	}
	//
	ic_conf_free(cm->conf);
	//
	free(cm);
}

//////////////////////////////////////////////////////////////////////////////////////
//
char * ic_url_response(struct ic_core_main * cm, char * args) {
	struct ic_mhash * mh_args;
	char * text;
	char * df;
	//
	struct ic_core_result * cr;
	//
	char * res = 0;
	/////////////////////////////////////
	args = trim(args);
	vardo(0, "ARGS: %s", args);
	mh_args = args_parse(args);
	//
	text = ic_mhash_get(mh_args, "text");
	df = ic_mhash_get(mh_args, "df");
	////////////////////////////////////////////
	if(text == NULL || df == NULL) {
		ic_mhash_free_all(mh_args);
		return res;
	}
	//
	text = urldecode(text);
	//
	vardo(0, "%s %s", df, text);
	cm->read_func = read_buf_one;
	cm->seg_func = word_seg;
	cm->show_func = get_cid_only;
	if(strcmp(df, "cid") == 0) {
		cm->show_func = get_cid_only;
	} else if(strcmp(df, "cid4") == 0) {
		cm->show_func = get_cid4;
	} else if(strcmp(df, "tag") == 0) {
		cm->show_func = get_tag;
	} else if(strcmp(df, "top24") == 0) {
		cm->show_func = get_top24;
	} else if(strcmp(df, "weight") == 0) {
		cm->show_func = get_weight;
	}
	//
	cr = ic_core_worker(cm, text);
	if(cr->output != NULL) {
		vardo(0, "%s", cr->output);
		//
		res = malloc(strlen(cr->output) + 1);
		memmove(res, cr->output, strlen(cr->output) + 1);
	}
	//
	free(text);
	ic_mhash_free_all(mh_args);
	ic_core_result_free(cr);
	//
	return res;
}
