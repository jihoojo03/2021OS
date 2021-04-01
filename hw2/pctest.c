#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <string.h>

int pipes[2];
int main(int argc, char *argv[]){
	int num;
	extern char *optarg;
	extern int optind;
	char *test_dir, *sol_c, *tar_c;
	int time_out, time_flag = 0, correct = 0, sum_time = 0, min_time = 100000, max_time = 0;
	struct timespec start, end, start_ch, end_ch;

	// Parameter Parsing
	while((num = getopt(argc, argv, "i:t:h")) != -1) {
		switch(num) {
			case 'i':
				test_dir = optarg;
				printf("It's testdir %s\n", test_dir);
				break;
			case 't':
				time_out = atoi(optarg);
				printf("It's timeout %d\n", time_out);
				break;
			case '?':
				printf("Wrong Flag!\n");
				return 0;
			case 'h':
			default:
				printf("Help Page\n Code Template : \n");
				printf("\tpctest -i <testdir> -t <timeout> <solution> <target> \n\n");
				printf("\t-i [string] : Set the testdir\n");
				printf("\t-t [integer] : Set the limit time\n");
				printf("\t-h : print help page\n");
				return 0;
		}
	}

	if (*argv[5]) {
		sol_c = argv[5];
	}
	if (*argv[6]) {
		tar_c = argv[6];
	}


	// Directory Access
	struct dirent *item;
	DIR *dp;
	int count = 0;

	dp = opendir(test_dir);
	if (dp != NULL){
		for(;;){
			item = readdir(dp);
			if(item == NULL)
				break;
			if(strstr(item -> d_name, "case") != NULL){
				printf("%s/%s\n", test_dir, item -> d_name);
				count++;
			}
		}
	}

	// Build Pipes
	if(pipe(pipes) != 0){
		perror("Error");
		exit(1);
	} 

	
	for(int i = 0; i < count; i++){

	// Build Paths
	char *s = malloc(sizeof(char) * 100);
	char num[10];
	strcpy(s, test_dir);
	strcat(s, "/case");
	sprintf(num, "%d", i+1);
	strcat(s, num);
	
	char *tar_ans = malloc(sizeof(char) * 100);
	char *sol_ans = malloc(sizeof(char) * 100);
	strcpy(tar_ans, "tar_ans");	strcpy(sol_ans, "sol_ans");
	strcat(tar_ans, num);		strcat(sol_ans, num);	

	// Make Process
	pid_t solution, target;
	solution = fork();
	clock_gettime(CLOCK_MONOTONIC, &start);

	if(solution == 0){
		int fd2 = open(s, O_RDONLY, 0644);
		char buf2[1000];
		dup2(fd2, 0);
		close(fd2);

		int fd = open(sol_ans, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		dup2(fd, 1);
		close(fd);

		execl("solution", "y", (char *) 0x0);
	}
	else{
		target = fork();
		if (target == 0){
			int fd2 = open(s, O_RDONLY, 0644);
			char buf2[1000];
			dup2(fd2, 0);
			close(fd2);
			
			close(pipes[0]);
			static long long ret = 0;		

			clock_gettime(CLOCK_MONOTONIC, &end);
			ret = 1000000000LL * (end.tv_sec - start.tv_sec) + (end.tv_nsec-start.tv_nsec);
			sprintf(buf2, "%lld", ret);
			write(pipes[1], buf2, 1000);
			close(pipes[1]);
	
			int fd = open(tar_ans, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			dup2(fd, 1);
			close(fd);
			
			execl("target", "y", (char *) 0x0);
		}
		else{
			pid_t term_pid;
			int exit_code;

			static long long ret = 0;
			
			term_pid = wait(&exit_code);
			term_pid = wait(&exit_code);

			clock_gettime(CLOCK_MONOTONIC, &end);
			ret = 1000000000LL * (end.tv_sec - start.tv_sec) + (end.tv_nsec-start.tv_nsec);

			char buf[1000];
			close(pipes[1]);
			read(pipes[0], buf, 1000);
			close(pipes[0]);
			ret -= atol(buf);
			printf("Case %d takes : %lldms\n", i+1 ,ret/1000);

			sum_time += ret/1000;
			if (min_time > ret/1000)
				min_time = ret/1000;
			if (max_time < ret/1000)
				max_time = ret/1000;			

			
			if (time_out * 1000 < ret/1000){
				printf("It is Timeout!\n");
				time_flag++;
			}

			int sol = open(sol_ans, O_RDONLY);
			int tar = open(tar_ans, O_RDONLY);

			if (sol > 0 && tar > 0){
				char *b1 = malloc(sizeof(char) * 100);
				char *b2 = malloc(sizeof(char) * 100);
				
				read(sol, b1, 1000);
				read(tar, b2, 1000);
				close(sol);
				close(tar);

				if(strcmp(b1, b2) == 0){
					fprintf(stdout, "%s\n", "The Answer is Right");
					correct++;
				}
				else{
					fprintf(stdout, "%s\n", "The Answer is Wrong");
				}
			}

		}

	}
	}

	printf("Sum Time : %d\nMin Time : %d\nMax Time : %d\n", sum_time, min_time, max_time);
	if (correct == count && time_flag == 0){
		printf("\n\tAll Succeded!\n");
	}else if(correct != count){
		printf("\n\tAnswer is Wrong!\n");
	}else{
		printf("\n\tTime is Over\n");
	}


	return 0;
}
