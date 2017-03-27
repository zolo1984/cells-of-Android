/*
 * wifi_host.h
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
#ifndef WIFI_HOST_H
#define WIFI_HOST_H

int init_wifi_proxy_host(void);
int create_wifi_proxy_socket(char *container_root);
int activate_wifi_proxy_socket(char *container_root, int init_pid);
int destroy_wifi_proxy_socket(char *container_root);

#endif
