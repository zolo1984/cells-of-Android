/*
 * =====================================================================================
 *
 *       Filename:  SwitchService.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年04月13日 15时13分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dr. Fritz Mehner (mn), mehner@fh-swf.de
 *        Company:  FH Südwestfalen, Iserlohn
 *
 * =====================================================================================
 */
#define _BSD_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#define LOG_TAG "Switchservice"
#include <cutils/log.h>

#define FILE_NAME_1 "/data/cells/cell1-rw/data/data/com.example.switchsystem/files/switchservice.txt"
#define FILE_NAME_2 "/data/cells/cell2-rw/data/data/com.example.switchsystem/files/switchservice.txt"
#define ACTIVE_PATH "/proc/dev_ns/active_ns_pid"
#define LOCKFILE "/data/lxc/mydaemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void daemonize(void)
{
	pid_t pid, sid;

	pid = fork();
	if (pid < 0) {
		printf("daemonize failed to fork.\n");
		exit(1);
	} else if (pid > 0) {
		printf("Sacrificial switchsystem child: exiting\n");
		exit(0);
	}

	/* Obtain a new process group */
	sid = setsid();
	if (sid < 0) {
		printf(stderr, "Failed to set sid\n");
		exit(1);
	}
	/*fprintf(stderr, "Closing all open FDs...\n");*/
	/*close_fds();*/
}


void prepare_file(void)
{
#if 0
	FILE *fp = NULL;

	fp = fopen(FILE_NAME_1, "w+");
	if(fp == NULL){
		printf("%s open with w failed!\n", FILE_NAME_1);
		return;
	}
	fclose(fp);

	fp = fopen(FILE_NAME_2, "w+");
	if(fp == NULL){
		printf("%s open with w failed!\n", FILE_NAME_2);
		return;
	}
	fclose(fp);

	fp = fopen("/data/cells/parse.txt", "w+");
	if(fp == NULL){
		printf("%s open with w failed!\n", "/data/cells/parse.txt");
		return;
	}
	fclose(fp);
#else
	system("rm -f " FILE_NAME_1);

	system("rm -f " FILE_NAME_2);

#endif
}


int StringFind(char *pSrc, char *pDst)
{

	int i, j;

	for(i = 0; pSrc[i]!='\0'; i++){

		if(pSrc[i] != pDst[0]){
			continue;
		}

		j = 0;

		while((pDst[j] != '\0') && (pSrc[i+j] != '\0')){

			j++;
			if(pDst[j] != pSrc[i + j]){
				break;
			}

		}
		if(pDst[j] == '\0'){
			return i;
		}

	}
	return -1;

}

int check_is_starting(void)
{
	FILE *fp = NULL;
	char *line = NULL, *p =NULL;
	ssize_t read = 0, len = 0;
	int count = 0;
	pid_t status;

	printf("check is starting !\n");

	status = system("fission list > /data/cells/parse.txt");
	if(status == -1){
		printf("system error in check is starting !\n");
		return -1;
	}

	fp = fopen("/data/cells/parse.txt", "r");
	if(fp == NULL){
		printf("%s open with w failed!", "/data/cells/parse.txt\n");
		return -1;
	}
	/* parse every line to find "running" */
	while((read = getline(&line, &len, fp)) != -1){
		printf("read the length of line = %d !\n", read);
		printf("%s !\n", line);
		printf("the first time :%s !\n", strtok(line, " "));
		if(StringFind(line, "running") > 0){
			fclose(fp);
			return 1;
		}

	}
	fclose(fp);
	return 0;

}

int create_fission_system(void)
{
	FILE *fp = NULL;
	char *line = NULL, *p =NULL;
	ssize_t read = 0, len = 0;
	int count = 0;
	pid_t status;
	int cell1_exist = 0;
	int cell2_exist = 0;

	printf("create cell1 and cell2 fissions !\n");

	status = system("fission list > /data/cells/parse.txt");
	if(status == -1){
		printf("system error in check is starting !\n");
		return -1;
	}

	fp = fopen("/data/cells/parse.txt", "r");
	if(fp == NULL){
		printf("%s open with w failed!", "/data/cells/parse.txt\n");
		return -1;
	}
	/* parse every line to find "running" */
	while((read = getline(&line, &len, fp)) != -1){
		printf("read the length of line = %d !\n", read);
		printf("%s !\n", line);
		printf("the first time :%s !\n", strtok(line, " "));
		if(StringFind(line, "cell1") > 0){
			printf("cell1 already exist");
			cell1_exist = 1;
			continue;
		}

		if(StringFind(line, "cell2") > 0){
			printf("cell2 already exist");
			cell2_exist = 1;
			continue;
		}
	}
	fclose(fp);

	if (0 == cell1_exist) {
		printf("cell1 not exist, create...");
		system("fission create cell1");
	}

	if (0 == cell2_exist) {
		printf("cell2 not exist, create...");
		system("fission create cell2");
	}

	system("sync");
	sleep(1);

	return 0;
}

void do_switch(void)
{
	char buf[100];

	printf("do switch!\n");
	snprintf(buf, sizeof(buf), "fission next");
	system(&buf);

}

void start_cell(int num)
{
	char buf[100];

	printf("start cell%d!\n", num);
	snprintf(buf, sizeof(buf), "fission start cell%d -D", num);
	system(&buf);

}

void switch_cell(int num)
{
	char buf[100];

	printf("switch cell%d!\n", num);
	snprintf(buf, sizeof(buf), "fission switch cell%d", num);
	system(&buf);

}

void MainLoop(void)
{
	int i = 0;
	FILE *fp1 = NULL, *fp2 = NULL;
	char flag[50];
	int var = 0, var1 = 0, var2 = 0;
	ssize_t size = 0;

	printf("MainLoop start!");

	create_fission_system();

	prepare_file();

	start_cell(1);

	switch_cell(1);

	while(1){


		fp1 = fopen(FILE_NAME_1, "rb");
		if(fp1 == NULL){
			printf("fopen %s failed! %d\n", FILE_NAME_1, errno);
			var1 = 0;
		}
		else{
			fread(flag, 1, 1, fp1);
			printf("read flag = %s!\n", flag);
			//fclose(fp);

			var1 = atoi(flag);
			printf("read var1 = %d!\n", var1);
			fclose(fp1);
			memset(flag, 0, sizeof(flag));

		}


		fp2 = fopen(FILE_NAME_2, "rb");
		if(fp2 == NULL){
			printf("fopen %s failed! %d\n", FILE_NAME_2, errno);
			var2 = 0;
		}
		else{
			fread(flag, 1, 1, fp2);
			printf("read flag = %s!\n", flag);
			//fclose(fp);

			var2 = atoi(flag);
			printf("read var2 = %d!\n", var2);
			fclose(fp2);
			memset(flag, 0, sizeof(flag));

		}

		for(i = 0; i < 2; i++){
			if(i == 0){
				var = var1;
			}
			else{
				var = var2;
			}
			switch(var){
				case 0x05:
					if(check_is_starting()){
						do_switch();
					}
					else{
						start_cell(2);
						do_switch();
					}
					/* note: atomic operation */
					prepare_file();
					break;

				default:
					printf("no switch request!\n");
					break;
			}
		}
		usleep(500000);

	}

}

int main(int argc, char ** argv)
{

	int sd;

	printf("main start!");
	sleep(5);

#if 0
	if(already_running(LOCKFILE)){
		printf("SwitchService already running!\n");
		return 0;
	}
#endif
	//daemonize();
	ALOGI("daemonize done!\n");
	MainLoop();
	/*never to here*/
	return 0;

}

