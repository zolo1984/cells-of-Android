/*
 * cell_config.c
 *
 * Cells configuration file manipulation
 *
 * Copyright (C) 2010-2013 Columbia University
 * Authors: Christoffer Dall <cdall@cs.columbia.edu>
 *          Jeremy C. Andrus <jeremya@cs.columbia.edu>
 *          Alexander Van't Hof <alexvh@cs.columbia.edu>
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
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define LOG_TAG "cellsconfig"
#include <cutils/log.h>
#include "cellsconfig.h"

static char *get_config_path(char *name)
{
	char *config_path = (char*)malloc(strlen(DEFL_CELL_DIR) + strlen(name) +
			    strlen(CONFIG_DIR) + 8); /* +8 for '//.conf\0' */
	if (config_path == NULL) {
		ALOGE("Failed to malloc for config_path: %s", strerror(errno));
		return NULL;
	}
	sprintf(config_path, "%s/%s/%s.conf", DEFL_CELL_DIR, CONFIG_DIR, name);
	return config_path;
}

static FILE *open_config(char *name, char *mode)
{
	char *config_path;
	FILE *fp;
	config_path = get_config_path(name);
	if (config_path == NULL)
		return NULL;

	fp = fopen(config_path, mode);
	if (fp == NULL) {
		ALOGE("Could not open %s: %s", config_path, strerror(errno));
		return NULL;
	}
	free(config_path);
	return fp;
}

struct conf_key_offset {
	char *key;
	int offset;
};

struct conf_key_offset config_keys[] = {
	{ "newcell",	offsetof(struct config_info, newcell) },
	{ "uts_ns",	offsetof(struct config_info, uts_ns) },
	{ "ipc_ns",	offsetof(struct config_info, ipc_ns) },
	{ "user_ns",	offsetof(struct config_info, user_ns) },
	{ "net_ns",	offsetof(struct config_info, net_ns) },
	{ "pid_ns",	offsetof(struct config_info, pid_ns) },
	{ "mount_ns",	offsetof(struct config_info, mount_ns) },
	{ "mnt_rootfs",	offsetof(struct config_info, mnt_rootfs) },
	{ "mnt_tmpfs",	offsetof(struct config_info, mnt_tmpfs) },
	{ "newpts",	offsetof(struct config_info, newpts) },
	{ "newcgrp",	offsetof(struct config_info, newcgrp) },
	{ "share_dalvik_cache",offsetof(struct config_info, share_dalvik_cache) },
	{ "sdcard_branch",	offsetof(struct config_info, sdcard_branch) },
	{ "autostart",	offsetof(struct config_info, autostart) },
	{ "autoswitch",	offsetof(struct config_info, autoswitch) },
	{ "wifiproxy",	offsetof(struct config_info, wifiproxy) },
	{ "initpid",	offsetof(struct config_info, initpid) },
	{ "id",		offsetof(struct config_info, id) },
	{ "console",	offsetof(struct config_info, console) },
};

static int *config_val_ptr(struct config_info *config, char *key)
{
	unsigned int i;
	struct conf_key_offset *ko;

	for (i = 0; i < sizeof(config_keys) / sizeof(*ko); i++) {
		ko = &config_keys[i];
		if (strcmp(ko->key, key) == 0)
			return (int *)((char *)config + ko->offset);
	}

	return NULL;
}

int read_config(char *name, struct config_info *config)
{
	char line[MAX_CONFIG_LINE_LEN];
	char scanf_fmt[25];
	char key[MAX_CONFIG_KEY_LEN];
	char value[MAX_MSG_LEN];
	int ret;
	FILE *fp;

	fp = open_config(name, "r");
	if (fp == NULL)
		return -1;

	/* init the config struct in case the config file is incomplete */
	memset(config, 0, sizeof(*config));
	config->newcell = 1;
	config->id = -1;
	config->initpid = -1;
	config->restart_pid = -1;

	/* Generate sscanf format string so that we can use MAX_MSG_LEN */
	/* aka a lot of complication just to utilize #defs */
	if (snprintf(scanf_fmt, 25, "%%%ds %%%d[^\n]s",
		     MAX_CONFIG_KEY_LEN-1, MAX_MSG_LEN) >= 25) {
		/* Never gonna happen... */
		ALOGE("How big are the #def LENs!? geeesh...");
		goto err_read_config;
	}

	/* Read through the configuration file */
	while (fgets(line, MAX_CONFIG_LINE_LEN, fp) != NULL) {
		ret = sscanf(line, scanf_fmt, key, value);
		if (ret != 2) {
			ALOGE("Invalid config line: %s", line);
			goto err_read_config;
		}

		int *ptr = config_val_ptr(config, key);
		if (!ptr) {
			ALOGE("Invalid config line: %s", line);
			goto err_read_config;
		}

		*ptr = atoi(value);
	}
	fclose(fp);
	return 0;

err_read_config:
	fclose(fp);
	return -1;
}
