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

int num_dir = 0;
int num_files = 0;
int num_finds = 0;

void find_string(char * file, char * ws){
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
			printf("File %s : Line %d : Word %s : Content %s\n", file, line_num, ws, buf);
			num_finds++;
		}
//		else
//			printf("No %s\n", ws); 
	}
	fclose(fp);
}

void find_dirs_files(char * dir, char * ws){

		if(mkfifo("channel", 0666)){
			if(errno != EEXIST){
				perror("fail to open fifo: ");
				exit(1);
			}
		}
	
//		int fd = open("channel", O_WRONLY | O_SYNC);
		printf("==");

		struct dirent *item;
		DIR * dp = opendir(dir);

		if (dp != NULL){
			for(;;){
				item = readdir(dp);
				if(item == NULL)
					break;
				else if(item->d_type == DT_DIR){	// Find Directories
					if(strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
						continue;
					num_dir++;
					char * next_dir = malloc(sizeof(char) * 100);	
					strcpy(next_dir, dir);
					strcat(next_dir, "/");
					strcat(next_dir, item->d_name);
					
//					write(fd, next_dir, strlen(next_dir));	
					printf("%s\n", next_dir);
//					printf("Next Dir : %s\n", next_dir);
				}
				else if(item->d_type == DT_REG){	// Find files
					num_files++;
					char * dir_file = malloc(sizeof(char) * 100);
					strcpy(dir_file, dir);
					strcat(dir_file, "/");
					strcat(dir_file, item->d_name);
//					printf("It's reg_file %s\n", dir_file);
					find_string(dir_file, "regcomp");
				}
			}
		}
//		close(fd);
}

int main(int argc, char *argv[]){
	
	int words = open("./words", O_RDONLY);
	char * ws = malloc(sizeof(char) * 100);
	read(words, ws, 100);
	close(words);
	printf("%s\n", ws);

	while(1){
		char * dir = malloc(sizeof(char) * 100);
		scanf("%s", dir);	
		find_dirs_files(dir, ws);
		printf("Dir : %d, Files : %d, Finds : %d\n", num_dir, num_files, num_finds);
		free(dir);
	}
	

	return 0;
}
