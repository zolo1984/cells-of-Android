#define _GNU_SOURCE
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cutils/properties.h>
#define LOG_TAG "VMCMD"
#include <cutils/log.h>
#include "cellsocket.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#include <linux/futex.h>
#include <sys/syscall.h>

static void exit_self(){
	int _e = VM_BASE_CMD<<16|SYSTEM_SWITCH;

	ALOGD("cell1 exit...");
	
	/*property_set("ctl.stop", "surfaceflinger");
	property_set("ctl.stop", "sensorservice");
	property_set("ctl.stop", "input");
	property_set("ctl.stop", "display");
	property_set("ctl.stop", "display.qservice");
	property_set("ctl.stop", "media.camera");*/
	system("stop");

	send_msg(0,sizeof(int),(const char *)&_e,NULL,NULL);

	ALOGD("cell1 exit end...");
};

static void* listen_self_daemon(void* o)
{
	o;

	property_set("persist.sys.exit", "0");

	volatile uint32_t* serialaddr = (volatile uint32_t*)__system_property_find("persist.sys.exit");
	uint32_t serial = __system_property_serial((const prop_info *)serialaddr);

    do {
		ALOGD("persist.sys.exit wait serial=%d",serial);
		syscall(__NR_futex, serialaddr, FUTEX_WAIT, serial, NULL);
		ALOGD("persist.sys.exit wait serialaddr=%d",*serialaddr);
    } while ((*serialaddr) == serial);

	exit_self();

	return (void*)0;
};

static void create_self_listen()
{
	int ret;
	pthread_t daemon_thread;
	/* Start listening for connections in a new thread */
	ret = pthread_create(&daemon_thread, NULL,listen_self_daemon, NULL);
	if (ret) {
		ALOGE("listen self pthread_create err: %s", strerror(errno));
	}
};

static void enter_self(){
	ALOGD("cell1 enter...");

	/*property_set("ctl.start", "surfaceflinger");
	property_set("ctl.start", "sensorservice");
	property_set("ctl.start", "input");
	property_set("ctl.start", "display");
	property_set("ctl.start", "display.qservice");
	property_set("ctl.start", "media.camera");*/
	system("start");

	create_self_listen();

	ALOGD("cell1 enter end...");
};

static void vm_handle_message(struct client_message * msg,const int cmd_flag)
{
	switch (cmd_flag)
	{
		case SYSTEM_SWITCH:
			enter_self();
		break;
	 default:
		break;
	}
};

static void init_net_work()
{
	sleep(2);
	system("ip addr add 172.16.1.4/16 dev wlan0");
	ALOGD("ip addr add 172.16.1.4/16 dev wlan0 errno=%s",strerror(errno));
	system("ip link set wlan0 up");
	ALOGD("ip link set wlan0 up errno=%s",strerror(errno));
	system("ip route add default via  172.16.1.254");
	ALOGD("ip route add default via  172.16.1.254 errno=%s",strerror(errno));
};

int main(int argc, char **argv)
{
	argc;argv;

	//init_net_work();

	register_cmd_handle(&vm_handle_message,VM_BASE_CMD);

	create_server_daemon(0);
	
    return 0;
};
