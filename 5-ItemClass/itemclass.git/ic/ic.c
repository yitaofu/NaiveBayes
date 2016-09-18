#include "ic_core.h"
#include "err/errdo.h"
#include <pthread.h>
#include <curl/curl.h>
//
//ic_worker_data
struct ic_worker_data {
	FILE * fp_input;
	FILE * fp_output;
	pthread_mutex_t mutex_input;
	pthread_mutex_t mutex_output;
	//
	char * input_format;
	char * output_format;
	//
	struct ic_core_main * cm;
};
/////////////////////////////////////////////////////////////////////////////////////////

//ic_worker
void * ic_worker(void * dat) {
	struct ic_worker_data * worker_data = (struct ic_worker_data *)dat;
	struct ic_core_result * cr;
	char buf[16384];
	char * res;
	char * flag;
	//
	while(1) {
		//
		flag = fgets(buf, sizeof(buf), worker_data->fp_input);
		if(flag == NULL) {
			break;
		}
		//
		if(strcmp(worker_data->input_format, "args") != 0) {
			cr = ic_core_worker(worker_data->cm, buf);
			if(cr->output) {
				fprintf(worker_data->fp_output, "%s\n", cr->output);
			}
			//
			ic_core_result_free(cr);
		} else {
			res = ic_url_response(worker_data->cm, buf);
			if(res != NULL) {
				fprintf(worker_data->fp_output, "%s\n", res);
				//
				free(res);
			}
		}
	}
	//
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
//
int main(int argc, char * argv[]) {
	struct ic_worker_data worker_data;
	struct ic_core_main * cm;
	//
	char * ic_conf_file = 0;
	//
	int i = 0;
	int worker_num = 8;
	pthread_t * pid_list;
	////////////////////////////////////////
	erdic(0);
	//
	if(argc < 4) {
		printf("demo: %s <conf> <input_format> <output_format> [worker_num=8] < <data>\n", argv[0]);
		exit(0);
	}
	////////////////////////////////////////////////////////////////////
	ic_conf_file = argv[1];
	//
	worker_data.input_format = argv[2];
	worker_data.output_format = argv[3];
	//
	if(argc >= 5) {
		worker_num = atoi(argv[4]);
	}
	/////////////////////////////////////////////////////
	vardo(1, "init start, worker_num %d", worker_num);
	cm = ic_core_init(ic_conf_file);
	vardo(1, "init end");
	///////////////////////////////
	curl_global_init(CURL_GLOBAL_ALL);
	//
	cm->seg_func = word_seg;
	//input_format
	if(strcmp(worker_data.input_format, "text") == 0) {
		cm->read_func = read_buf_one;
	} else if(strcmp(worker_data.input_format, "bat") == 0) {
		cm->read_func = read_buf_bat;
	} else if(strcmp(worker_data.input_format, "words") == 0) {
		cm->read_func = read_buf_bat;
		cm->seg_func = word_explode;
	} else {
		cm->read_func = read_buf_one;
	}
	//output_format
	if(strcmp(worker_data.output_format, "top24") == 0) {
		cm->show_func = get_top24;
	} else if(strcmp(worker_data.output_format, "bat") == 0) {
		cm->show_func = get_bat;
	} else if(strcmp(worker_data.output_format, "cid") == 0) {
		cm->show_func = get_cid_only;
	} else if(strcmp(worker_data.output_format, "cid4") == 0) {
		cm->show_func = get_cid4;
	} else if(strcmp(worker_data.output_format, "tag") == 0) {
		cm->show_func = get_tag;
	} else if(strcmp(worker_data.output_format, "weight") == 0) {
		cm->show_func = get_weight;
	} else {
		cm->show_func = get_top24;
	}
	/////////
	errdo((pid_list = (pthread_t *)malloc(sizeof(pthread_t) * worker_num)) == NULL, "");
	//
	worker_data.fp_input = stdin;
	worker_data.fp_output = stdout;
	worker_data.cm = cm;
	//
	pthread_mutex_init(&(worker_data.mutex_input), NULL);
	pthread_mutex_init(&(worker_data.mutex_output), NULL);
	/////////////////////////////////
	for(i = 0; i < worker_num; i ++) {
		pthread_create(&(pid_list[i]), NULL, ic_worker, &worker_data);
	}
	//
	for(i = 0; i < worker_num; i ++) {
		pthread_join(pid_list[i], NULL);
	}
	//
	pthread_mutex_destroy(&(worker_data.mutex_input));
	pthread_mutex_destroy(&(worker_data.mutex_output));
	//
	curl_global_cleanup();
	//
	ic_core_main_free(cm);
	//
	erdie();
	return 0;
}
