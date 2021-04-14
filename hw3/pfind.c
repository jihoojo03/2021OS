#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

typedef struct node{
	char * name;
	struct node * link;
}NODE;

typedef struct{
	NODE *front;
	NODE *rear;
	int length;
}QUEUE;

QUEUE * Q_Create(){
	QUEUE *q = (QUEUE *)malloc(sizeof(QUEUE));
	q -> front = NULL;
	q -> rear = NULL;
	q -> length = 0;
	return q;
}

void Q_Free(QUEUE * q){
	NODE * temp_q;
	while(q->front != NULL){
		temp_q = q->front;
		q->front = q->front->link;
		free(temp_q);
	}
	free(q);
}

void Q_Insert(QUEUE * q, char * n){
	NODE * temp_node = (NODE *)malloc(sizeof(NODE));
	temp_node -> name = n;
	temp_node -> link = NULL;

	if(q->front == NULL){
		q->front = temp_node;
	}
	else{
		q->rear->link = temp_node;
	}
	q->rear = temp_node;
	q->length++;
}

char * Q_Delete(QUEUE * q){
	if(q->front == NULL)
		return "Nothing";
	else{
		char * pop = q->front->name;
		NODE * temp_q = q->front;
		q->front = q->front->link;
		free(temp_q);
		q->length--;

		if(q->front == NULL)
			q->rear = NULL;

		return pop;
	}
}

int Q_Length(QUEUE * q){
	return q->length;
}

char * make_execute(char * binary){
	char * exe_name = malloc(sizeof(char) * strlen(binary));
	strcpy(exe_name, binary);

	for(int i = 0; i < strlen(binary); i++){
		if(binary[i] == '.'){
			exe_name[i] = '\0';
			return exe_name; 
		}
	}
}

void binary_to_execute(char * binary){
	char * exe_name = make_execute(binary);

	pid_t exe_file;
	exe_file = fork();
	if(exe_file == 0){
		execl("/usr/bin/gcc", "gcc", "-o", exe_name, binary, (char *) 0x0);
	}
	else{
		wait(0);
	}
}

void find_files(char * dir){
	
	QUEUE * dir_q = Q_Create();
	Q_Insert(dir_q, dir);

	while(Q_Length(dir_q) != 0){
		char * d = Q_Delete(dir_q);
		printf("%d\n", Q_Length(dir_q));
		struct dirent *item;
		DIR * dp = opendir(d);

		if (dp != NULL){
			for(;;){
				item = readdir(dp);
				if(item == NULL)
					break;
				else if(item->d_type == DT_DIR){
					if(strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
						continue;
					char * next_dir = malloc(sizeof(char) * 100);	
					strcpy(next_dir, d);
					strcat(next_dir, "/");
					strcat(next_dir, item->d_name);
					Q_Insert(dir_q, next_dir);
				}
				else if(item->d_type == DT_REG){
					printf("It's reg_dir %s\n", d);
					printf("It's reg_file %s\n", item->d_name);
				}
			}
		}
	}
}

void signal_handler(int sig){
	if (sig == SIGINT)
		printf("Interrupt!\n");
	else if (sig == SIGKILL)
		printf("Die\n");
	else if (sig == SIGCHLD)
		printf("My child is dead\n");
}

int pipes[2];
int main(int argc, char *argv[]){

	// Make Process
	pid_t solution, target;

	binary_to_execute("worker.c");
	char * exe_name = make_execute("worker.c");
	printf("%s\n", exe_name);

	find_files("/usr/local/llvm");

	if(mkfifo("channel", 0666)){
		if(errno != EEXIST){
			perror("fail to open fifo: ");
			exit(1);
		}
	}

	int fd = open("channel", O_RDONLY | O_SYNC); 
	printf("===");
	char buff[1024];
	
	target = fork();
	if(target == 0){
		execl(exe_name, exe_name, (char *) 0x0);
	}
	else{
//		memset(buff, 0, 1024);
//		read(fd, buff, 1024);
//		printf("Received : %s\n", buff);

		int exit_code = 0;
		signal(SIGCHLD, signal_handler);
		wait(&exit_code);
	}
	close(fd);


	return 0;
}
