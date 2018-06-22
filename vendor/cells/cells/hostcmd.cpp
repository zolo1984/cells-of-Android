#define _GNU_SOURCE
#include <errno.h>
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
#define LOG_TAG "HOSTCMD"
#include <cutils/log.h>
#include "util.h"
#include "cellsocket.h"
#include "cellsconfig.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#include <linux/futex.h>
#include <sys/syscall.h>

#include <binder/IServiceManager.h>
#include <gui/ISurfaceComposer.h>
#include <utils/String16.h>

#include <powermanager/IPowerManager.h>
#include <powermanager/PowerManager.h>

static pthread_mutex_t _switch_mutex_t;

static int vm_init = 0;

static int is_TSystem(const char *path)
{
	struct stat buf;
	if (stat(path, &buf) < 0)
		return 0;
	return S_ISREG(buf.st_mode);
}

static void  exit_self(bool bstart)
{
	ALOGD("host exit...");

	pthread_mutex_lock(&_switch_mutex_t);

	{
		android::sp<android::IServiceManager> sm = android::defaultServiceManager();
		android::sp<android::ISurfaceComposer> mComposer = 
			android::interface_cast<android::ISurfaceComposer>(sm->checkService(android::String16("SurfaceFlinger")));
		if(mComposer != NULL){
			mComposer->exitSelf();
		}
	}

	{
		property_set("ctl.stop", "adbd");
	}

	system("cellc switch cell1");

	if(!bstart){
		int _e = VM_BASE_CMD<<16|SYSTEM_SWITCH_ENTER;
		send_msg(1,sizeof(int),(const char *)&_e,NULL,NULL); //exit_self_end
	}

	pthread_mutex_unlock(&_switch_mutex_t);

	ALOGD("host exit end...");
};

static void* exit_self_end(void* o)
{
	o;

	ALOGD("host exit_self_end...");

	pthread_mutex_lock(&_switch_mutex_t);

	{
		android::sp<android::IServiceManager> sm = android::defaultServiceManager();
		android::sp<android::IPowerManager> mPowerManager = 
			android::interface_cast<android::IPowerManager>(sm->checkService(android::String16("power")));
		if(mPowerManager != NULL){
			mPowerManager->goToSleep(long(ns2ms(systemTime())),GO_TO_SLEEP_REASON_POWER_BUTTON,0);
		}
	}

	pthread_mutex_unlock(&_switch_mutex_t);

	ALOGD("host exit_self_end end...");

	return (void*)0;
};

static void create_exit_self_end_pthread()
{
	int ret;
	pthread_t daemon_thread;
	
	/* Start listening for connections in a new thread */
	ret = pthread_create(&daemon_thread, NULL,exit_self_end, NULL);
	if (ret) {
		ALOGE("create_exit_self_end_pthread err: %s", strerror(errno));
	}
};

static void stop_self()
{
	ALOGD("host stop_self...");

	pthread_mutex_lock(&_switch_mutex_t);

	//system("stop");

	pthread_mutex_unlock(&_switch_mutex_t);

	ALOGD("host stop_self end...");
};

static void* listen_stop_othersystem_daemon(void* o)
{
	o;

	volatile uint32_t* serialaddr = (volatile uint32_t*)__system_property_find("persist.sys.stop.othersystem");
	uint32_t serial = __system_property_serial((const prop_info *)serialaddr);

	do {
		ALOGD("persist.sys.stop.othersystem wait serial=%d",serial);
		syscall(__NR_futex, serialaddr, FUTEX_WAIT, serial, NULL);
		ALOGD("persist.sys.stop.othersystem wait serialaddr=%d",*serialaddr);

		char value[PROPERTY_VALUE_MAX];
		property_get("persist.sys.stop.othersystem", value, "0");
		if(strcmp(value, "0") == 0){
			return (void*)0;
		}
	} while ((*serialaddr) == serial);

	{//to the other system
		int _e = VM_BASE_CMD<<16|SYSTEM_STOP_SELF;
		send_msg(1,sizeof(int),(const char *)&_e,NULL,NULL); //other system stop_self()
	}

	return (void*)0;
}

static void *listen_self_daemon(void* o)
{
	o;

	volatile uint32_t* serialaddr = (volatile uint32_t*)__system_property_find("persist.sys.exit");
	uint32_t serial = __system_property_serial((const prop_info *)serialaddr);

	do {
		ALOGD("persist.sys.exit wait serial=%d",serial);
		syscall(__NR_futex, serialaddr, FUTEX_WAIT, serial, NULL);
		ALOGD("persist.sys.exit wait serialaddr=%d",*serialaddr);

		char value[PROPERTY_VALUE_MAX];
		property_get("persist.sys.exit", value, "0");
		if(strcmp(value, "0") == 0){
			serial = *serialaddr;
		}
	} while ((*serialaddr) == serial);

	exit_self(false);

	return (void*)0;
};

static void create_self_listen()
{
	int ret;
	pthread_t daemon_thread;
	pthread_t stop_othersystem_daemon_daemon_thread;

	property_set("persist.sys.exit", "0");

	property_set("persist.sys.stop.othersystem", "0");

	if(!is_TSystem("/data/cells/cell1/.cell"))
		return;

	/* Start listening for connections in a new thread */
	ret = pthread_create(&daemon_thread, NULL,listen_self_daemon, NULL);
	if (ret) {
		ALOGE("listen_self_daemon pthread_create err: %s", strerror(errno));
	}

	ret = pthread_create(&stop_othersystem_daemon_daemon_thread, NULL,listen_stop_othersystem_daemon, NULL);
	if (ret) {
		ALOGE("listen_stop_othersystem_daemon pthread_create err: %s", strerror(errno));
	}
};


static void enter_self(bool bstart)
{
	ALOGD("host enter...");

	pthread_mutex_lock(&_switch_mutex_t);

	system("cellc switch host");

	system("start");

	create_self_listen();

	if(!bstart){
		int _e = VM_BASE_CMD<<16|SYSTEM_SWITCH_EXIT;
		send_msg(1,sizeof(int),(const char *)&_e,NULL,NULL); //other system exit_self_end()
	}

	{
		android::sp<android::IServiceManager> sm = android::defaultServiceManager();
		android::sp<android::IPowerManager> mPowerManager = 
			android::interface_cast<android::IPowerManager>(sm->checkService(android::String16("power")));
		if(mPowerManager != NULL){
			mPowerManager->wakeUp(long(ns2ms(systemTime())),android::String16("enter_self"),android::String16(""));
		}
	}

	{
		property_set("ctl.start", "adbd");
	}

	{
		android::sp<android::IServiceManager> sm = android::defaultServiceManager();
		android::sp<android::ISurfaceComposer> mComposer = 
			android::interface_cast<android::ISurfaceComposer>(sm->checkService(android::String16("SurfaceFlinger")));
		if(mComposer != NULL){
			mComposer->enterSelf();
		}
	}

	pthread_mutex_unlock(&_switch_mutex_t);

	ALOGD("host enter end...");
};

static void vm_init_ok()
{
	vm_init = 1;
};

static void host_handle_message(struct client_message * msg,const int cmd_flag)
{
	switch (cmd_flag)
	{
	case SYSTEM_SWITCH_ENTER:
		enter_self(false);
		break;
	case SYSTEM_SWITCH_EXIT:
		create_exit_self_end_pthread();
		break;
	case SYSTEM_STOP_SELF:
		stop_self();
		break;
	case SYSTEM_VM_INIT_OK:
		vm_init_ok();
		break;
	default:
		break;
	}
};

static void thread_exit_handler(int sig)
{
	sig;

	pthread_exit(0);
}

int main()
{
	signal(SIGPIPE, SIG_IGN);

	struct sigaction actions;
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = thread_exit_handler;
	if (sigaction(SIGQUIT|SIGKILL, &actions, NULL) < 0){
		ALOGE("sigaction error: %s",strerror(errno));
	}

	pthread_mutex_init(&_switch_mutex_t,NULL);

	register_cmd_handle(&host_handle_message,HOST_BASE_CMD);

	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.exit", value, "1");
	if((strcmp(value, "0") == 0)){
		enter_self(true);
	}else{
		exit_self(true);
	}

	create_server_daemon(1);
	return 0;
};