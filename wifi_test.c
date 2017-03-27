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
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <linux/socket.h>
//#include <hardware_legacy/wifi_proxy.h>

/*
 * Open connection with wifi_proxy and return file descriptor.
 */
static int open_conn(void)
{
	int sd;
	struct sockaddr_un addr;
	size_t addr_len;

	/* Establish connection */
	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("Failed to connect to celld");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	//addr->sun_path[0] = 0; /* Make abstract */
	strcpy(&addr.sun_path[0], WIFI_PROXY_SOCKET);
	addr_len = strlen(WIFI_PROXY_SOCKET) + 1 + offsetof(struct sockaddr_un, sun_path);
	if (connect(sd, (struct sockaddr *) &addr, addr_len) != 0) {
		if (errno == ENOENT)
			fprintf(stderr, "Failed to connect to wifi_proxy: wifi_proxy " \
					"does not appear to be running\n");
		else
			perror("Failed to connect to wifi_proxy");
		return -1;
	}
	return sd;
}

static int send_buf(int fd, void *data, size_t len)
{
	int ret, remain = len;
	char *buf = data;
	while (remain > 0) {
		if ((ret = write(fd, buf, remain)) <= 0)
			return -1;
		buf += ret;
		remain -= ret;
	}
	return 0;
}

static int send_command(int sd, WIFI_PROXY_CMD cmd)
{
	int rev = 0;

    /* send msg num */
    rev = cmd;
	if (send_buf(sd, &rev, sizeof(rev))) {
		perror("sending msg num fail!");
		return -1;
	}

    if ((cmd == START_SUPPLICANT) || (cmd == STOP_SUPPLICANT)) {
        /* send msg length */
        rev = sizeof(int);
        if (send_buf(sd, &rev, sizeof(rev))) {
    		perror("sending msg length fail!");
    		return -1;
    	}

        /* send msg */
        rev = 1;
        if (send_buf(sd, &rev, sizeof(rev))) {
    		perror("sending msg length fail!");
    		return -1;
    	}

    } else {
        /* send msg length */
        rev = 0;
        if (send_buf(sd, &rev, sizeof(rev))) {
    		perror("sending msg length fail!");
    		return -1;
    	}
    }
    
	return 0;
}

int main(int argc, const char **argv)
{
    int fd = 0;
    int cmd;
    
    printf("[wifi_test] argc[%d] argv[1] = %s\n", argc, argv[1]);

    if (argc != 2)
        return -1;

    cmd = atoi(argv[1]);
    if ((cmd < LOAD_DRIVER) || (cmd > ENSURE_ENTROPY_FILE)) {
        printf("[wifi_test] cmd[%d] error!\n", cmd);
        return -1;
    }
    
    fd = open_conn();
    if (fd < 0)
        return -1;

    send_command(fd, (WIFI_PROXY_CMD)cmd);
    
    close(fd);
    return 0;
}

