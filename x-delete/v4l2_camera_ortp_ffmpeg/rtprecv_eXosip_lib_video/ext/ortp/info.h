/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __INFO_H__
#define __INFO_H__

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

void info_htable_display(void);
void info_key_value_display(void);

char *info_key_value_get(char *key);
bool info_key_value_set(char *key, char *value);

// loopback ipaddr get and set
char *info_loopback_ipaddr_get(void);
bool  info_loopback_ipaddr_set(char *value);

// local ipaddr get and set
char *info_local_ipaddr_get(void);
bool  info_local_ipaddr_set(char *value);

// default ipaddr get and set
char *info_default_hwaddr_get(void);
char *info_default_ipaddr_get(void);
char *info_default_netmask_get(void);

bool  info_default_hwaddr_set(char *value);
bool  info_default_ipaddr_set(char *value);
bool  info_default_netmask_set(char *value);

// audio local ipaddr and port get and set
int   info_audio_local_port_get(void);
char *info_audio_local_ipaddr_get(void);

bool  info_audio_local_port_set(int value);
bool  info_audio_local_ipaddr_set(char *value);

// audio remote ipaddr and port get and set
int   info_audio_remote_port_get(void);
char *info_audio_remote_ipaddr_get(void);

bool  info_audio_remote_port_set(int value);
bool  info_audio_remote_ipaddr_set(char *value);

// video local ipaddr and port get and set
int   info_video_local_port_get(void);
char *info_video_local_ipaddr_get(void);

bool  info_video_local_port_set(int value);
bool  info_video_local_ipaddr_set(char *value);

// video remote ipaddr and port get and set
int   info_video_remote_port_get(void);
char *info_video_remote_ipaddr_get(void);

bool  info_video_remote_port_set(int value);
bool  info_video_remote_ipaddr_set(char *value);


// exosip from ipaddr get and set
char *info_exosip_from_ipaddr_get(void);
bool  info_exosip_from_ipaddr_set(char *value);

// exosip from port get and set
int  info_exosip_from_port_get(void);
bool info_exosip_from_port_set(int value);

// exosip to ipaddr get and set
char *info_exosip_to_ipaddr_get(void);
bool  info_exosip_to_ipaddr_set(char *value);

// exosip to port get and set
int  info_exosip_to_port_get(void);
bool info_exosip_to_port_set(int value);

// exosip proxy ipaddr get and set
char *info_exosip_proxy_ipaddr_get(void);
bool  info_exosip_proxy_ipaddr_set(char *value);

// exosip proxy port get and set
int  info_exosip_proxy_port_get(void);
bool info_exosip_proxy_port_set(int value);

// exosip route ipaddr get and set
char *info_exosip_route_ipaddr_get(void);
bool  info_exosip_route_ipaddr_set(char *value);

// exosip contact ipaddr get and set
char *info_exosip_contact_ipaddr_get(void);
bool  info_exosip_contact_ipaddr_set(char *value);

// exosip expires time get and set
int   info_exosip_expires_time_get(void);
bool  info_exosip_expires_time_set(int value);

// exosip local username get and set
char *info_exosip_local_username_get(void);
bool  info_exosip_local_username_set(char *value);

// exosip local password get and set
char *info_exosip_local_password_get(void);
bool  info_exosip_local_password_set(char *value);

// exosip remote username get and set
char *info_exosip_remote_username_get(void);
bool  info_exosip_remote_username_set(char *value);

// exosip remote password get and set
char *info_exosip_remote_password_get(void);
bool  info_exosip_remote_password_set(char *value);

// exosip transport ipaddr get and set
char *info_exosip_transport_ipaddr_get(void);
bool  info_exosip_transport_ipaddr_set(char *value);

// exosip transport port get and set
int   info_exosip_transport_port_get(void);
bool  info_exosip_transport_port_set(int value);

// exosip transport ipproto get and set (i.e IPPROTO_UDP or IPPROTO_TCP)
int   info_exosip_transport_ipproto_get(void);
bool  info_exosip_transport_ipproto_set(int value);


#endif /* __INFO_H__ */


