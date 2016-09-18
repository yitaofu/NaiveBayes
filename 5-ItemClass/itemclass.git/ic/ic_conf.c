#include "err/errdo.h"
#include "ic_mhash.h"
#include "ic_tools.h"
#include "ic_mem.h"
#include "read_conf.h"
#include <unistd.h>
#include "ic_core.h"
//
#define conf_item_box(item_name, def_value, item_type) \
	{#item_name, def_value, offset(struct ic_conf, item_name), item_type}

/////////////////////////////////////////////////////////////////////////////////////
//配置项
static struct conf_item conf_item_list[] = {//配置项名称，默认值，配置项类型
	//分词相关
	conf_item_box(word_seg_basic, "", CONF_TYPE_STRING),
	conf_item_box(word_seg_split, " AND ", CONF_TYPE_STRING),
	conf_item_box(word_exp_split, " ", CONF_TYPE_STRING),
	//默认类型
	conf_item_box(cid_name_def, "DEF", CONF_TYPE_STRING),
	//标签相关
	conf_item_box(tag_num_max, "11", CONF_TYPE_SIZE),
	conf_item_box(tag_other_cid4, "", CONF_TYPE_STRING),
	//词典路径
	conf_item_box(bayes_dict_path, "bayes.dict", CONF_TYPE_PATH),
	conf_item_box(ratio_dict_path, "ratio.dict", CONF_TYPE_PATH),
	conf_item_box(cid_dict_path, "cid.dict", CONF_TYPE_PATH),
	conf_item_box(tag_dict_path, "cid4_tag.list", CONF_TYPE_PATH),
	conf_item_box(tag_name_path, "tag_name.list", CONF_TYPE_PATH)
};

////////////////////////
//构造配置结构
struct ic_conf * ic_conf_build(char * conf_file) {
	struct ic_conf * conf = 0;
	struct ic_mhash * mh_conf = 0;
	//
	errdo((conf = (struct ic_conf *)malloc(sizeof(struct ic_conf))) == NULL, "");
	//读取配置
	mh_conf = read_conf(conf_file);
	//
	conf = conf_build(conf, conf_item_list, sizeof(conf_item_list) / sizeof(struct conf_item), mh_conf, CONF_PRI_DEF);
	//
	return conf;
}

//配置释放
void ic_conf_free(struct ic_conf * conf) {
	if(conf) {
		//
		free(conf);
	}
}

//配置展示
void ic_conf_view(struct ic_conf * conf, FILE * fp) {
	conf_view(conf, conf_item_list, sizeof(conf_item_list) / sizeof(struct conf_item), fp);
}
