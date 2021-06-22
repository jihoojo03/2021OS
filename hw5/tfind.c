#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

int num_key = 0;
int num_files = 0;
int num_finds = 0;

char * tar_dir = NULL;
char ** keywords = NULL;

struct timespec start, end;
static long long ret = 0;

pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER; // For num_files
pthread_mutex_t find_lock = PTHREAD_MUTEX_INITIALIZER; // For num_finds

//Concurrent Queue
typedef struct __node_t{
	char dir[512];
	struct __node_t *next;
} node_t;


typedef struct __queue_t{
	node_t *head;
	node_t *tail;
	pthread_mutex_t h_lock, t_lock;
} queue_t;

void Queue_Init(queue_t *q){
	node_t *tmp = malloc(sizeof(node_t));
	tmp->next = NULL;
	q->head = q->tail = tmp;
	pthread_mutex_init(&q->h_lock, NULL);
	pthread_mutex_init(&q->t_lock, NULL);
}

void Queue_Enqueue(queue_t *q, char * str){
	node_t *tmp = malloc(sizeof(node_t));
	assert (tmp != NULL);
	strcpy(tmp->dir, str);
	tmp->next = NULL;

	pthread_mutex_lock(&q->t_lock);
	q->tail->next = tmp;
	q->tail = tmp;
	pthread_mutex_unlock(&q->t_lock);
}

char * Queue_Dequeue(queue_t *q){
	pthread_mutex_lock(&q->h_lock);
	node_t *tmp = q->head;
	node_t *new_head = tmp->next;
	if(new_head == NULL){
		pthread_mutex_unlock(&q->h_lock);
		return NULL;
	}

	char * new_dir = (char *)malloc(sizeof(char) * 512);
	strcpy(new_dir, new_head -> dir);
	q->head = new_head;
	pthread_mutex_unlock(&q->h_lock);
	free(tmp);
	return new_dir;
}

// Make Concurrent Q
queue_t dir_Q;

// signal handling
void signal_handler(int sig){
	if (sig == SIGINT){
		printf("I'm died by Ctrl+C\n");

		// Print Summary
		printf(" ===================\n Total files : %d \n Total Find Lines : %d \t\n", num_files, num_finds);
		clock_gettime(CLOCK_MONOTONIC, &end);
		long time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
		printf(" Total Time (Milli) : %lf\n ===================\n", (double)time/1000000);

		exit(0);
	}
}

// Find keywords within String
void find_string(char * file, char ** ws, char * th_name){
	FILE *fp;
	char buf[1024];
	int line_num = 0;

	fp = fopen(file, "r");
	if(fp == NULL){
		fprintf(stderr, "No file %s\n", file);
		printf("Total files : %d \t:\t Total Find Lines : %d \t:\t \n", num_files, num_finds);
		exit(1);
	}
	
	while(fgets(buf, 1024, fp)){
		int keypoint = 0;
		line_num++;	

		for(int i = 0; i < num_key; i++){
			if(strstr(buf, ws[i])){
				keypoint++;
			}
		}
		
		if(keypoint == num_key){
			// Mutex Number of Finds
			pthread_mutex_lock(&find_lock);
				printf("File --> %s \t:\t Line --> %d \t:\t %s \t\n", file, line_num, buf);
				num_finds++;
			pthread_mutex_unlock(&find_lock);
		}
	}
	fclose(fp);
}

// Find Subdirectories and Files in the Dirctory with Concurrent Manner
void find_dirs_files(char * dir, char ** ws, char * th_name){
	struct dirent * item;
	DIR * dp = opendir(dir);

	if (dp != NULL){
		for(;;){
			item = readdir(dp);
			if(item == NULL)
				break;
			else if(item->d_type == DT_DIR){
				if(strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
					continue;

				char * next_dir = malloc(sizeof(char) * 512);
				strcpy(next_dir, dir);	
				strcat(next_dir, "/");	
				strcat(next_dir, item->d_name);

				Queue_Enqueue(&dir_Q, next_dir);
				free(next_dir);	
			}
			else if(item->d_type == DT_REG){
				char * dir_file = malloc(sizeof(char) * 512);
				strcpy(dir_file, dir);	
				strcat(dir_file, "/");	
				strcat(dir_file, item->d_name);

				// MUTEX Number of Files
				pthread_mutex_lock(&file_lock);
					num_files++;
				pthread_mutex_unlock(&file_lock);

				find_string(dir_file, ws, th_name);
				free(dir_file);
			}
		}
	}
}

void * thread_find(void * ptr){
	char * name = (char *) ptr;
	char * dir_name;

	while(1){
		dir_name = Queue_Dequeue(&dir_Q);
		if(dir_name == NULL)		// There is NO Entry in the waiting Q
			break;
		find_dirs_files(dir_name, keywords, name);
	}
}


int main(int argc, char *argv[]){

	// Initialize
	pthread_t th[16];
	
	int num = 0;
	int flag_t = 1;		// Default
	char * thname = (char*)malloc(sizeof(char)* 16);

	Queue_Init(&dir_Q);
	clock_gettime(CLOCK_MONOTONIC, &start);

	signal(SIGINT, signal_handler);

	// Option Setting
	while((num = getopt(argc, argv, "t:")) != -1){
		switch(num){
			case 't':
				flag_t = atoi(optarg);
				if(flag_t < 1 || flag_t > 16){
					printf("Num of threads can be between 1 to 16 \n");
					exit(0);
				}
		}
	}

	tar_dir = argv[optind];

	num_key = argc - optind - 1;
	if(num_key <= 0){
		printf("Please put the keywords!\n");
		exit(0);
	}

	printf(" =================\n Target dir : %s\n", tar_dir);
	printf(" Number of threads : %d \n ================== \n\n", flag_t);

	keywords = (char **)malloc(sizeof(char *) * num_key);
	for(int i = 0; i < num_key; i++){
		keywords[i] = (char*)malloc(sizeof(char)* 512);
	}

	for(int i = optind + 1; i < argc; i++){
		strcpy(keywords[i + num_key - argc], argv[i]);
	}

	// Put the first root target directory
	Queue_Enqueue(&dir_Q, tar_dir);

	// 'tfind' with Concurrent Manner
	for(int i = 0; i < flag_t; i++){
		pthread_create(&(th[i]), NULL, thread_find, NULL);
	}

	for(int i = 0; i < flag_t; i++){
		pthread_join(th[i], NULL);
	}

	// Print Summary
	printf(" ===================\n Total files : %d \n Total Find Lines : %d \t\n", num_files, num_finds);

	clock_gettime(CLOCK_MONOTONIC, &end);
	long time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
	printf(" Total Time (Milli) : %lf\n ===================\n", (double)time/1000000);

	return 0;
}









































