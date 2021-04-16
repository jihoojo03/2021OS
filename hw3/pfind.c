#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

int num_dir = 0;
int num_files = 0;
int num_finds = 0;
int cur_num_dir = 1;
int dir_count = 0;
int w_num = 2;		// Worker Default
struct timespec start, end;
static long long ret = 0;

void show_result(){
	FILE *fr;
	int dir, file, find;
	fr = fopen("result.txt", "r");
	fscanf(fr, "%d %d %d", &dir, &file, &find);
	printf("Dir : %d, File : %d, Find : %d\n", dir, file, find);
	fclose(fr);
}

void signal_handler(int sig){
	if (sig == SIGINT){
		exit(0);
	}
	else if (sig == SIGKILL){
		exit(0);
	}
	else if (sig == SIGCHLD){
		dir_count++;
		if(dir_count >= w_num){
			clock_gettime(CLOCK_MONOTONIC, &end);
			ret = 1000000000LL * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
			printf("\nRunning time : %lldms\n", ret/1000);
			show_result();
			exit(0);
		}
	}
}

void reset_count(){
	FILE *f;
	f = fopen("count.txt", "w");
	fprintf(f, "%d", 0);
	fclose(f);
}

int get_count(){
	FILE *f;
	int c = 0;
	f = fopen("count.txt", "r");
	fscanf(f, "%d", &c);
	return c; 
}

void upcount(){
	FILE *fi; FILE *fr;
	int c;
	fr = fopen("count.txt", "r");
	fscanf(fr, "%d", &c);
	fclose(fr);

	c++;
	fi = fopen("count.txt", "w");
	fprintf(fi, "%d\n", c);
	fclose(fi);
}

void reset_result(){
	FILE *f;
	f = fopen("result.txt", "w");
	fprintf(f, "%d %d %d", 0, 0, 0);
	fclose(f);
}

void write_result(){
	FILE *fi; FILE *fr;
	int dir, file, find;
	fr = fopen("result.txt", "r");
	fscanf(fr, "%d %d %d", &dir, &file, &find);
	fclose(fr);

	fi = fopen("result.txt", "w");
	fprintf(fi, "%d %d %d\n", num_dir + dir, num_files + file, num_finds + find);
	fclose(fi);
	cur_num_dir = num_dir + dir;
	num_dir = 0; num_files = 0; num_finds = 0;
}	

int write_bytes(int fd, void * a, int len){
	char * s = (char *) a;
	int i = 0;
	while(i < len){
		int b;
		b = write(fd, s + i, len - i);
		if (b == 0)	break;
		i += b;
	}
	return i;
}

int read_bytes(int fd, void * a, int len){
	char * s = (char *) a;
	int i;

	for (i = 0; i < len; ){
		int b;
		b = read(fd, s + i, len - i);
		if (b == 0)
			break;
		i += b;
	}
	return i;
}

void find_string(char * file, char * ws, int flag_a){
	FILE *fp;
	char buf[1024];
	int line_num = 0;

	fp = fopen(file, "r");
	if(fp == NULL){
		fprintf(stderr, "No file %s\n", file);
		exit(1);
	}
	
	while(fgets(buf, 1024, fp)){
		line_num++;
		if(strstr(buf, ws)){
			char r_link[1024];
			int len;

			if(realpath(file, r_link) == NULL){
				perror("realpath error");
				exit(0);
			}

			if(flag_a == 1)
				printf("File %s : Line %d : Word %s : Content %s\n", r_link, line_num, ws, buf);
			else
				printf("File %s : Line %d : Word %s : Content %s\n", file, line_num, ws, buf);
			num_finds++;
		}
	}
	fclose(fp);
}

void find_dirs_files(char * dir, char * ws, int flag_a){
	struct dirent * item;
	DIR * dp = opendir(dir);

	if(mkfifo("back_fifo", 0666)){
		if(errno != EEXIST){
			perror("fail to open fifo: ");
		}
	}

	if (dp != NULL){
		for(;;){
			item = readdir(dp);
			if(item == NULL)
				break;
			else if(item->d_type == DT_DIR){
				if(strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
					continue;

				num_dir++;
				char * next_dir = malloc(sizeof(char) * 100);
				strcpy(next_dir, dir);
				strcat(next_dir, "/");
				strcat(next_dir, item->d_name);

				int fd = open("back_fifo", O_WRONLY | O_SYNC);
				
				char s [128];
				size_t len = 0;

				while(len < 128){
					char c;
					c = next_dir[len];
					if ((c==EOF || c=='\n') && (len > 0))
						break;
					s[len++] = c;
				}

				if(write_bytes(fd, &len, sizeof(len)) != sizeof(len))
					break;
			
				if(write_bytes(fd, s, len) != len)
					break;

				close(fd);
			}
			else if(item->d_type == DT_REG){
				num_files++;
				char * dir_file = malloc(sizeof(char) * 100);
				strcpy(dir_file, dir);
				strcat(dir_file, "/");
				strcat(dir_file, item->d_name);

				find_string(dir_file, ws, flag_a);
			}
		}
	}
}



void make_workers(int num, char * tar_dir, char * keyword, int flag_a){
	pid_t worker;
	int i;

	signal(SIGINT, signal_handler);
	clock_gettime(CLOCK_MONOTONIC, &start);

	for(i = 0; i < num; i++){

	worker = fork();
	if(worker == 0){
		if (i == 0)	
			find_dirs_files(tar_dir, keyword, flag_a);

		while(get_count() < cur_num_dir){

			if(mkfifo("front_fifo", 0666)){
				if(errno != EEXIST){
					perror("fail to open fifo: ");
				}
			}
				
			int fd = open("front_fifo", O_RDONLY | O_SYNC);
			char s[128];
			size_t len, bs;

			while(1){
				flock(fd, LOCK_EX);	

				if(read_bytes(fd, &len, sizeof(len)) != sizeof(len)){
					flock(fd, LOCK_UN);
					break;
				}
	
				bs = read_bytes(fd, s, len);
				flock(fd, LOCK_UN);
		
				if(bs != len)	break;
			}

			close(fd);
			find_dirs_files(s, keyword, flag_a);

			write_result();
			upcount();
		}
		
		printf("%dth child I'm Gone!\n", i + 1);
		kill(getpid(), SIGINT);

	}
	else if (worker > 0){
		signal(SIGCHLD, signal_handler);

		if(mkfifo("back_fifo", 0666)){
			if(errno != EEXIST){
				perror("fail to open fifo: ");
			}
		}

		int fd = open("back_fifo", O_RDONLY | O_SYNC);


		while(1){
			char s[128];
			size_t len, bs;

			flock(fd, LOCK_EX);	

			if(read_bytes(fd, &len, sizeof(len)) != sizeof(len)){
				flock(fd, LOCK_UN);
				break;
			}

			bs = read_bytes(fd, s, len);
			flock(fd, LOCK_UN);

			if(mkfifo("front_fifo", 0666)){
				if(errno != EEXIST){
					perror("fail to open fifo: ");
				}
			}

			int fd_f = open("front_fifo", O_WRONLY | O_SYNC);

			char w [128];
			size_t len_t = 0;

			while(len_t < 128){
				char c;

				c = s[len_t];
				if ((c==EOF || c=='\n') && (len_t > 0))
					break;
				w[len_t++] = c;
			}

			if(write_bytes(fd_f, &len, sizeof(len_t)) != sizeof(len_t))
				break;
			
			if(write_bytes(fd_f, w, len_t) != len_t)
				break;

			close(fd_f);

			if(bs != len)	break;
		}

		close(fd);

		wait(0);
	}
	}
}


int main(int argc, char *argv[]){

	int num = 0;
	int flag_p = 0, flag_c = 0, flag_a = 0;
	char * tar_dir = NULL;
	char * keyword = NULL;

	// Option Setting
	while((num = getopt(argc, argv, "acp:")) != -1){
		switch(num){
			case 'a':
				flag_a = 1;
				break;
			case 'c':
				flag_c = 1;
				break;
			case 'p':
				flag_p = 1;
				w_num = atoi(optarg);
				if(w_num < 1 || w_num > 10){
					printf("Num of worker can be between 1 to 10\n");
					exit(0);
				}
		}
	}	

	
	// Get the target directory and keywords
	if(argc <= 5){
		tar_dir = (char*)malloc(sizeof(char) * strlen(argv[optind]));
		strcpy(tar_dir, argv[optind]);
		keyword = (char*)malloc(sizeof(char) * strlen(argv[optind + 1]));
		strcpy(keyword, argv[optind + 1]);
	}
	else{
		printf("Please write correctly, ./pfind [<option>} <dir> [<keyword>]\n");
		exit(0);
	}

	// Make IO Reset
	reset_count();
	reset_result();

	// Make Workers
	make_workers(w_num, tar_dir, keyword, flag_a);

	return 0;
}
