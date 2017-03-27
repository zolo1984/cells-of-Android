/*
 * wifi_host.c
 *
 * user space proxy routines for WPA supplicant daemon
 *
 * Copyright (C) 2010-2013 Columbia University
 * Authors: Christoffer Dall <cdall@cs.columbia.edu>
 *          Jeremy C. Andrus <jeremya@cs.columbia.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <linux/socket.h>

#define LOG_TAG "Cells/wifihost"
#include <cutils/log.h>
#include <cutils/memory.h>
#include <cutils/misc.h>
#include <hardware_legacy/wifi_inner.h>
#include <hardware_legacy/wifi.h>
#include <hardware_legacy/wifi_proxy.h>

#include "celld.h"
#include "socket_common.h"

static int host_active;
static int refresh_fd_set;

static struct client_list client_list = { .head = NULL, .tail = NULL };

struct wifi_client_data {
	char socket_path[PATH_MAX];
	int socket;
	pthread_t listen_thread;
	int should_stop;
};
//#define ALOGD printf
//#define ALOGE printf
//#define ALOGI printf

#define client_data(x) ((struct wifi_client_data *)x->data)

struct client_message {
	uint32_t cmd_socket;
	uint32_t msg_num;
	uint32_t msg_len;
	void *msg;
};

#if 1
/*
 * Handle a message, forward to wpa_supplicant and send reply on the socket
 */
void *handle_client_message(void *opaque)
{
	char *buf, *buf_alloc;
	int *reply;   /* The protocol reply value */
	int *ret_val; /* The function return value field */
	int *ret_len; /* The return message length field */
	int ret;
	size_t left;
	struct client_message *client_msg = (struct client_message *)opaque;

	buf = buf_alloc = calloc(WIFI_MAX_RPLY_LEN, 1);
	if (!buf) {
		ALOGE("wifi_host malloc buf: %s", strerror(errno));
		goto out_free;
	}

	/*
	 * Initialize the pointers into the reply buffer and assume for now we're
	 * sending on OK reposnse left with 0 message length and a return value.
	 */
	reply = (int *)buf;
	ret_len = (int *)buf + 1;
	ret_val = (int *)buf + 2;
	buf += 3 * sizeof(int);
	*reply = 0;
	*ret_len = 0;
    ALOGD("[handle_client_message] msg_num = %d\n", client_msg->msg_num);
	switch (client_msg->msg_num) {
	case LOAD_DRIVER:
		ALOGD("wifi_load_driver");
		*ret_val = real_wifi_load_driver();
		break;
	case UNLOAD_DRIVER:
		ALOGD("wifi_unload_driver");
		*ret_val = real_wifi_unload_driver();
		break;
	case IS_LOADED:
		ALOGD("is_wifi_driver_loaded");
		*ret_val = real_is_wifi_driver_loaded();
		break;
	case START_SUPPLICANT: {
		int p2p_supported;
		if (client_msg->msg_len != sizeof(int)) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		p2p_supported = *(int *)(client_msg->msg);
		ALOGD("wifi_start_supplicant(%d)", p2p_supported);
		*ret_val = real_wifi_start_supplicant(p2p_supported);
	} break;
	case STOP_SUPPLICANT: {
		int p2p_supported;
		if (client_msg->msg_len != sizeof(int)) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		p2p_supported = *(int *)(client_msg->msg);
		ALOGD("wifi_stop_supplicant(%d)", p2p_supported);
		*ret_val = real_wifi_stop_supplicant(p2p_supported);
	} break;
	case CONNECT_TO_SUPPLICANT: {
		int nmlen = 0;
		char *iface = NULL;
		if (client_msg->msg_len < sizeof(int)) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		nmlen = *(int *)(client_msg->msg);
		if (client_msg->msg_len < (sizeof(int) + nmlen) ||
		    nmlen < 1) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		if (nmlen == 1) /* just the NULL byte! */
			iface = NULL;
		else
			iface = (char *)(client_msg->msg) + sizeof(int);
		ALOGD("wifi_connect_to_supplicant(%s)", iface);
		*ret_val = real_wifi_connect_to_supplicant(iface);
	} break;
	case CLOSE_SUPPLICANT_CONNECTION: {
		int nmlen = 0;
		char *iface;
		if (client_msg->msg_len < sizeof(int)) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		nmlen = *(int *)(client_msg->msg);
		if (client_msg->msg_len < (sizeof(int) + nmlen) ||
		    nmlen < 1) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		if (nmlen == 1) /* just the NULL byte! */
			iface = NULL;
		else
			iface = (char *)(client_msg->msg) + sizeof(int);
		ALOGD("wifi_close_supplicant_connection(%s)", iface);
		real_wifi_close_supplicant_connection(iface);
		*ret_val = 0;
	} break;
	case WAIT_FOR_EVENT: {
		int event_len, nmlen;
		char *ptr, *iface;
        int i;

		if (client_msg->msg_len < 2*sizeof(int)) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		nmlen = *(int *)(client_msg->msg);
		event_len = *(((int *)(client_msg->msg)) + 1);
        ALOGE("nmlen[%p-%d] event_len[%p-%d]\n", client_msg->msg, nmlen, ((int *)(client_msg->msg)) + 1, event_len);
        for (i = 0; i < client_msg->msg_len; i++) {
            printf("%x-", ((unsigned char*)client_msg->msg)[i]);
        }
        ALOGE("\n");
		if (client_msg->msg_len < (2*sizeof(int) + nmlen) ||
		    nmlen < 1) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		if (nmlen == 1)
			iface = NULL;
		else
			iface = (char *)(client_msg->msg) + 2*sizeof(int);

		if (event_len <= 0 ||
		    event_len > (WIFI_MAX_RPLY_LEN - 12)) {		    
			ALOGE("wifi proxy ERROR on LINE:%d event_len[%d]", __LINE__, event_len);
			*reply = -1;
			break;
		}

		ptr = (char *)buf;
		ALOGD("wifi_wait_for_event(%s), len:%d", iface, event_len);
		*ret_val = real_wifi_wait_for_event(iface, ptr, event_len);
		if (*ret_val <= 0 || *ret_val > WIFI_MAX_RPLY_LEN) {
			ALOGE("Can't handle event size: %d", *ret_val);
			*reply = -1;
			break;
		} else {
			/* We have some wifi event data to send back */
			*ret_len = *ret_val;
		}
	} break;
	case COMMAND: {
		char *command, *reply_buf;
		size_t reply_len, nmlen;
		char *iface;

		/* reply_len(int) + nmlen(int) + at least 2 bytes of command */
		if (client_msg->msg_len < 2*sizeof(int) + 2) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		nmlen = *(int *)(client_msg->msg);
		reply_len = *((int *)(client_msg->msg) + 1);
		if (client_msg->msg_len < (2*sizeof(int) + nmlen + 2) ||
		    nmlen < 1) {
			ALOGE("wifi proxy ERROR on LINE:%d", __LINE__);
			*reply = -1;
			break;
		}
		if (nmlen == 1)
			iface = NULL;
		else
			iface = (char *)(client_msg->msg) + 2*sizeof(int);

		/* validate the reply_len */
		//if (reply_len == 0 || reply_len > (WIFI_MAX_RPLY_LEN - 12)) {
		if (reply_len > (WIFI_MAX_RPLY_LEN - 12)) {
			*reply = -1;
			ALOGE("reply_len %d exceeds maximum allowed: %u",
			      (int)reply_len, WIFI_MAX_RPLY_LEN);
			break;
		}

		/* the command follows all the other data */
		command = (char *)(client_msg->msg) + 2*sizeof(int) + nmlen;

		/* Execute the command now */
		reply_buf = buf;
		ALOGD("wifi_command(%s): '%s' (len:%d)\n", iface, command, reply_len);
		*ret_val = real_wifi_command(iface, command, reply_buf, &reply_len);
		/* Validate the number of bytes in the buffer (new len value) */
		if (reply_len > (WIFI_MAX_RPLY_LEN - 12)) {
			ALOGE("Cannot handle wifi command reply size: %u", reply_len);
			*reply = -1;
			break;
		} else {
			/* Let's send back the data from the command */
			*ret_len = reply_len;
		}
	} break;
	case CHANGE_FW_PATH:
		ALOGD("changing firmware path %s\n", (const char *)client_msg->msg);
        *ret_val = real_wifi_change_fw_path((const char *)client_msg->msg);
		break;
	case ENSURE_ENTROPY_FILE:        
		ALOGD("ensure entroy file....\n");
        *ret_val = ensure_entropy_file_exists_inner();
		break;
#ifdef CONFIG_MEDIATEK_WIFI_BEAM       
    case GET_MAC_ADDR:        
		ALOGD("wifi_get_own_addr");
        *ret_val = real_wifi_get_own_addr(buf);
        *ret_len = 17;
        break;
#endif        
	default:
		ALOGE("unknown wifi command: %d", client_msg->msg_num);
		*reply = -1;
		break;
	}

	/* Figure out how many bytes to send */
	if (*reply < 0)
		left = sizeof(int);
	else
		left = 3 * sizeof(int) + *ret_len;

    ALOGD("[handle_client_message] cmd[%d] reply[%d] ret_val = %d\n", client_msg->msg_num, *reply, *ret_val);
	/* Send the now prepared reply */
	buf = buf_alloc;
	while (left > 0) {
		ret = send(client_msg->cmd_socket, buf, left, 0);
		if (ret <= 0) {
			ALOGE("wifi_host send reply: "
			      "ERROR(%d) %s", errno, strerror(errno));
			break;
		}
		left -= ret;
		buf += ret;
	}

out_free:
	free(buf_alloc);
    sleep(2);
	close(client_msg->cmd_socket);
	free(client_msg->msg);
	free(client_msg);

	return (void *)0; /* Unused */
}

/*
 * Listen for incoming connections and handle them one by one
 *
 * It's a bit rough, but we really expect all data to come through more or less
 * at once.
 */
static void *listen_client_socket(void *opaque)
{
	int ret, cmd_socket;
	struct sockaddr_un remote;
	int msg_num;
	size_t left, msg_len;
	socklen_t len;
	void *message, *ptr;
	pthread_t handle_thread;
	struct client_message *client_message;
	struct socket_client *client = (struct socket_client *)opaque;
	struct wifi_client_data *client_data = client_data(client);

	ALOGI("Listening for libhardware:wifi connections on '%s'",
	      client_data->socket_path);

#define check_stop() \
	if (client_data->should_stop) \
		goto out_err

	/* OK, we're ready to accept some incoming connections */
	if (listen(client_data->socket, 5) == -1) {
		ALOGE("listen client_data->socket: %s", strerror(errno));
		goto out_err;
	}

	/* Main accept loop */
	for (;;) {
		check_stop();

		/* Let's get a buffer for the data */
		message = calloc(WIFI_MAX_MSG_LEN, 1);
		if (!message) {
			ALOGE("malloc message buffer: %s", strerror(errno));
			goto out_err;
		}

		len = sizeof(remote);
		/* Block and wait for one */
		cmd_socket = accept(client_data->socket,
				    (struct sockaddr *)&remote, &len);
		if (cmd_socket < 0) {
			ALOGE("wifi_proxy accept: %s", strerror(errno));
			free(message);
			goto out_err;
		}
		check_stop();

		/* Caught a fish - now fetch and validate the protocol fields */
		ret = recv(cmd_socket, (void *)&msg_num, 4, 0);
		if (ret != 4) {
			ALOGE("recv msg_num: %s", strerror(errno));
			free(message);
			close(cmd_socket);
			continue;
		}
		check_stop();

		if (msg_num < 0 || msg_num >= MAX_MSG_NUM) {
			ALOGE("Unexpected msg_num value: %d", msg_num);
			free(message);
			close(cmd_socket);
			continue;
		}

		ret = recv(cmd_socket, (void *)&msg_len, 4, 0);
		if (ret != 4) {
			ALOGE("recv msg_len: %s", strerror(errno));
			free(message);
			close(cmd_socket);
			continue;
		}
		check_stop();

		if (msg_len > WIFI_MAX_MSG_LEN) {
			ALOGE("Unexpected msg_len value: %u", msg_len);
			free(message);
			close(cmd_socket);
			continue;
		}

		/* This may not come all at once, so receive in chunks */
		left = msg_len;
		while (left > 0) {
			ptr = (char *)message + (msg_len - left);
			ret = recv(cmd_socket, ptr, left, 0);
			if (ret <= 0) {
				ALOGE("recv message: %s", strerror(errno));
				free(message);
				close(cmd_socket);
				break;
			}
			check_stop();

			left -= ret;
		}

		/*
		 * If we received the entire message then go ahead and process
		 * it in its own thread and let the thread close the socket
		 * (and free the message memory) when it's done with it.
		 */
		if (left == 0) {
			client_message = calloc(sizeof(*client_message), 1);
			if (!client_message) {
				ALOGE("wifi_host malloc client_message: "
				      "ERROR(%d) %s", errno, strerror(errno));
				free(message);
				close(cmd_socket);
				goto out_err;
			}

			client_message->cmd_socket = cmd_socket;
			client_message->msg_num = msg_num;
			client_message->msg_len = msg_len;
			client_message->msg = message;

			check_stop();
			/* TODO: keep track of these threads */
			ret = pthread_create(&handle_thread, NULL,
					     handle_client_message, (void *)client_message);
			if (ret) {
				ALOGE("wifi_host pthread_create handle_thread: "
				      "ERROR(%d) %s", errno, strerror(errno));
				free(message);
				free(client_message);
				close(cmd_socket);
				goto out_err;
			}
		} else {
			ALOGE("Could not receive entire message "
			      "(missing %u bytes)", left);
			free(message);
			close(cmd_socket);
		}
	}

out:
	return (void *)0;

out_err:
	/* remove this thread from the client list */
	ALOGE("Removing %s from client list", client_data->socket_path);
	del_socket_client(client, &client_list);
	close(client_data->socket);
	free(client_data);
	free(client);
	/*
	 * TODO: kill pthreads belonging to this client that may be blocking
	 * e.g. for a WIFI event.
	 */
	return (void *)-1;
}

/*
 * Creates the socket and registers it with the internal bookkeeping. Each call
 * will create a new thread that accepts new connections and handles incoming
 * data. Remember to close destroy the wifi host before exiting the program.
 */
int create_wifi_proxy_socket(char *container_root)
{
	struct socket_client *client;
	struct wifi_client_data *client_data;
	struct sockaddr_un local;
	int sock, len, ret;

	client = calloc(sizeof(*client), 1);
	if (!client) {
		ALOGE("malloc client: %s", strerror(errno));
		return -1;
	}

	client_data = calloc(sizeof(*client_data), 1);
	if (!client) {
		ALOGE("malloc client_data: %s", strerror(errno));
		goto out_err_free_client;
	}

	snprintf(client_data->socket_path, PATH_MAX, "%s/%s",
		 container_root, WIFI_PROXY_SOCKET);

	ALOGI("Creating wifi proxy socket at: %s", client_data->socket_path);

	client_data->socket = cell_socket(client_data->socket_path);
	if (client_data->socket == -1)
		ALOGE("create wifi socket: %s", strerror(errno));

	client->data = client_data;
	strncpy(client->root_path, container_root, PATH_MAX);
	client->init_pid = -1;
	add_socket_client(client, &client_list);

printf("aaaaaaa\n");
	/* Start listening for connections in a new thread */
	ret = pthread_create(&client_data->listen_thread, NULL,
			     listen_client_socket, (void *)client);
	if (ret) {
		ALOGE("wifi_host pthread_create: %s", strerror(errno));
		goto out_err_free_client_data;
	}

	return client_data->socket;

out_err_free_client_data:
	del_socket_client(client, &client_list);
	free(client_data);
out_err_free_client:
	free(client);
	return -1;
}

/*
 * Tell the wifi_host which PID is the init process of the container with the
 * specified root. This is used to test if the device_namespace is active and
 * therefore allowed to modify device settings.
 */
int activate_wifi_proxy_socket(char *container_root, int init_pid)
{
	struct socket_client *client =
		find_socket_client(container_root, &client_list);
	if (!client)
		return -1;

	client->init_pid = init_pid;
	return 0;
}

static void kill_wifi_client(struct socket_client *client)
{
	int sd, len;
	struct sockaddr_un remote;
	struct wifi_client_data *client_data = client_data(client);

	client_data->should_stop = 1;

	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0) {
		ALOGE("cannot open socket: %s", strerror(errno));
		return;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, client_data->socket_path);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
	if (connect(sd, (struct sockaddr *)&remote, len) < 0) {
		ALOGE("cannot connect: %s", strerror(errno));
		return;
	}

	close(sd);
}

/*
 * Find the wifi client structure for the passed container path and stop it's
 * socket thread, make sure to close the socket and remove the client from
 * the list of clients.
 */
int destroy_wifi_proxy_socket(char *container_root)
{
	int ret, retval;
	struct socket_client *client;
	struct wifi_client_data *client_data;

	client = find_socket_client(container_root, &client_list);
	if (!client) {
		ALOGE("Could not find wifi client: %s\n", container_root);
		return -1;
	}
	client_data = client_data(client);

	/* Found it, let's clean things up */
	del_socket_client(client, &client_list);

	if (!client_data) {
		free(client);
		return 0;
	}

	kill_wifi_client(client);

	return 0;
}

int init_wifi_proxy_host(void)
{
	pthread_mutex_init(&client_list.mutex, NULL);
	return 0;
}
#else
int init_wifi_proxy_host(void){return 0;}
int create_wifi_proxy_socket(char *container_root) {return 0;}
int activate_wifi_proxy_socket(char *container_root, int init_pid) {return 0;}
int destroy_wifi_proxy_socket(char *container_root) {return 0;}
#endif

