/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Second generation work queue implementation
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/if.h>
 
#include "info.h"
#include "initcall.h"
#include "gdbus/gdbus.h"

static GHashTable *info_htable = NULL;

static void __init_0 info_htable_init(void)
{
	info_htable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	pr_info("%s: info_htable = %p\n", __func__, info_htable);
}

static void __exit_0 info_htable_deinit(void)
{
	if (!info_htable) {
		return;
	}

	pr_info("%s: info_htable = %p\n", __func__, info_htable);
	g_hash_table_destroy(info_htable);
	
	info_htable = NULL;
}

/*
static bool info_htable_insert(char *key, char *value)
{
	char *i_key = NULL;
	char *i_value = NULL;

	if (!info_htable) {
		return false;
	}

	i_key = g_strdup(key);
	i_value = g_strdup(value);
	
	return g_hash_table_insert(info_htable, i_key, i_value);
}
*/

static bool info_htable_replace(char *key, char *value)
{
	char *i_key = NULL;
	char *i_value = NULL;

	if (!info_htable) {
		return false;
	}
	
	i_key = g_strdup(key);
	i_value = g_strdup(value);
	
	return g_hash_table_replace(info_htable, i_key, i_value);
}

static char *info_htable_lookup(char *key)
{
	char *i_key = key;
	
	if (!info_htable) {
		return NULL;
	}

	return g_hash_table_lookup(info_htable, i_key);
}


char *info_key_value_get(char *key)
{
	return info_htable_lookup(key);
}

bool info_key_value_set(char *key, char *value)
{
	return info_htable_replace(key, value);
}

char *info_loopback_ipaddr_get(void)
{
	return info_htable_lookup("lo:ipaddr");
}

bool info_loopback_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("lo:ipaddr", value);
}

char *info_local_ipaddr_get(void)
{
	return info_htable_lookup("local:ipaddr");
}

bool info_local_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("local:ipaddr", value);
}

char *info_default_hwaddr_get(void)
{
	return info_htable_lookup("default:hwaddr");
}

bool info_default_hwaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("default:hwaddr", value);
}

char *info_default_ipaddr_get(void)
{
	return info_htable_lookup("default:ipaddr");
}

bool info_default_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("default:ipaddr", value);
}

char *info_default_netmask_get(void)
{
	return info_htable_lookup("default:netmask");
}

bool info_default_netmask_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("default:netmask", value);
}

int info_audio_local_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("audio:lport");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_audio_local_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("audio:lport", port);
}

char *info_audio_local_ipaddr_get(void)
{
	return info_htable_lookup("audio:lipaddr");
}

bool info_audio_local_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("audio:lipaddr", value);
}

int info_audio_remote_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("audio:rport");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_audio_remote_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("audio:rport", port);
}

char *info_audio_remote_ipaddr_get(void)
{
	return info_htable_lookup("audio:ripaddr");
}

bool info_audio_remote_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("audio:ripaddr", value);
}

int info_video_local_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("video:lport");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_video_local_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("video:lport", port);
}

char *info_video_local_ipaddr_get(void)
{
	return info_htable_lookup("video:lipaddr");
}

bool info_video_local_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("video:lipaddr", value);
}

int info_video_remote_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("video:rport");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_video_remote_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("video:rport", port);
}

char *info_video_remote_ipaddr_get(void)
{
	return info_htable_lookup("video:ripaddr");
}

bool info_video_remote_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("video:ripaddr", value);
}



char *info_exosip_from_ipaddr_get(void)
{
	return info_htable_lookup("exosip:from:ipaddr");
}

bool info_exosip_from_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:from:ipaddr", value);
}

int info_exosip_from_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("exosip:from:port");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_exosip_from_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("exosip:from:port", port);
}

char *info_exosip_to_ipaddr_get(void)
{
	return info_htable_lookup("exosip:to:ipaddr");
}

bool info_exosip_to_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:to:ipaddr", value);
}

int info_exosip_to_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("exosip:to:port");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_exosip_to_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("exosip:to:port", port);
}

char *info_exosip_proxy_ipaddr_get(void)
{
	return info_htable_lookup("exosip:proxy:ipaddr");
}

bool info_exosip_proxy_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:proxy:ipaddr", value);
}

int info_exosip_proxy_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("exosip:proxy:port");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_exosip_proxy_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("exosip:proxy:port", port);
}


char *info_exosip_route_ipaddr_get(void)
{
	return info_htable_lookup("exosip:route");
}

bool info_exosip_route_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:route", value);
}

char *info_exosip_contact_ipaddr_get(void)
{
	return info_htable_lookup("exosip:contact");
}

bool info_exosip_contact_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:contact", value);
}

int info_exosip_expires_time_get(void)
{
	int expires = -1;
	char *value = info_htable_lookup("exosip:expires");

	if (value) {
		expires = atoi(value);
	}

	return expires;
}

bool info_exosip_expires_time_set(int value)
{
	char expires[32];

	if (value < 0) {
		return false;
	}
	memset(expires, 0, sizeof(expires));
	sprintf(expires, "%d", value);

	return info_htable_replace("exosip:expires", expires);
}

char *info_exosip_local_username_get(void)
{
	return info_htable_lookup("exosip:lusername");
}

bool info_exosip_local_username_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:lusername", value);
}

char *info_exosip_local_password_get(void)
{
	return info_htable_lookup("exosip:lpassword");
}

bool info_exosip_local_password_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:lpassword", value);
}


char *info_exosip_remote_username_get(void)
{
	return info_htable_lookup("exosip:rusername");
}

bool info_exosip_remote_username_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:rusername", value);
}


char *info_exosip_remote_password_get(void)
{
	return info_htable_lookup("exosip:rpassword");
}

bool info_exosip_remote_password_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:rpassword", value);
}

int info_exosip_transport_port_get(void)
{
	int port = -1;
	char *value = info_htable_lookup("exosip:port");

	if (value) {
		port = atoi(value);
	}

	return port;
}

bool info_exosip_transport_port_set(int value)
{
	char port[32];

	if (value < 0) {
		return false;
	}
	memset(port, 0, sizeof(port));
	sprintf(port, "%d", value);

	return info_htable_replace("exosip:port", port);
}

char *info_exosip_transport_ipaddr_get(void)
{
	return info_htable_lookup("exosip:ipaddr");
}

bool info_exosip_transport_ipaddr_set(char *value)
{
	if (!value) {
		return false;
	}

	return info_htable_replace("exosip:ipaddr", value);
}

// IPPROTO_UDP or IPPROTO_TCP
int info_exosip_transport_ipproto_get(void)
{
	int transport = -1;
	char *value = info_htable_lookup("exosip:ipproto");

	if (value) {
		transport = atoi(value);
	}

	return transport;
}

// IPPROTO_UDP or IPPROTO_TCP
bool info_exosip_transport_ipproto_set(int value)
{
	char transport[32];

	if (value < 0) {
		return false;
	}
	memset(transport, 0, sizeof(transport));
	sprintf(transport, "%d", value);

	return info_htable_replace("exosip:ipproto", transport);
}


static void local_ipaddr_get(void)
{
    int fd, intrface;    
    struct ifreq buf[4];
    struct ifconf ifc;
    struct ifreq *ifreq;
    struct sockaddr_in *in;
    int loopback = 0;
    int defaultipaddr = 1;
    int defaulthwaddr = 1;
    int defaultnetmask = 1;

    char *key, *value;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        return;
    }

    ifc.ifc_req = buf;
    ifc.ifc_len = sizeof(buf);
    
    if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) != 0) {
        goto err_io;
    }

    ifreq = ifc.ifc_req;
    intrface = ifc.ifc_len / sizeof(struct ifreq);

    for (int i = 0; i < intrface; i++, ifreq++) {
    		loopback = 0;
    		
        if (ioctl(fd, SIOCGIFFLAGS, ifreq) == 0) {
            if (!(ifreq->ifr_flags & IFF_UP)) {
                continue;
            }

            if (ifreq->ifr_flags & IFF_LOOPBACK) {
            	loopback = 1;
        			// continue;
            } 
        }
       
				if (ioctl(fd, SIOCGIFHWADDR, ifreq) == 0) {
					key = g_strdup_printf("%s:hwaddr", ifreq->ifr_name);
					value = g_strdup_printf("%02x:%02x:%02x:%02x:%02x:%02x", 
																	ifreq->ifr_hwaddr.sa_data[0],
																	ifreq->ifr_hwaddr.sa_data[1],
																	ifreq->ifr_hwaddr.sa_data[2],
																	ifreq->ifr_hwaddr.sa_data[3],
																	ifreq->ifr_hwaddr.sa_data[4],
																	ifreq->ifr_hwaddr.sa_data[5]);
					info_htable_replace(key, value);
					g_free(key);
					
					if (!loopback && defaulthwaddr) {
            	defaulthwaddr = 0;
            	
            	key = g_strdup_printf("default:hwaddr");
            	info_htable_replace(key, value);
            	g_free(key);
          }
          
          g_free(value);
				}

        if (ioctl(fd, SIOCGIFADDR, ifreq) == 0) {
            in = (struct sockaddr_in *)&ifreq->ifr_addr;
            value = inet_ntoa(in->sin_addr);
            
            key = g_strdup_printf("%s:ipaddr", ifreq->ifr_name);
            info_htable_replace(key, value);
            g_free(key);
            
            if (!loopback && defaultipaddr) {
            	defaultipaddr = 0;
            	
            	key = g_strdup_printf("default:ipaddr");
            	info_htable_replace(key, value);
            	g_free(key);
            }
        }
        
        if (ioctl(fd, SIOCGIFNETMASK, ifreq) == 0) {
            in = (struct sockaddr_in *)&ifreq->ifr_netmask;
            value = inet_ntoa(in->sin_addr);
            
            key = g_strdup_printf("%s:netmask", ifreq->ifr_name);
            info_htable_replace(key, value);
            g_free(key);
            
            if (!loopback && defaultnetmask) {
            	defaultnetmask = 0;
            	
            	key = g_strdup_printf("default:netmask");
            	info_htable_replace(key, value);
            	g_free(key);
            }
        }      
    }


err_io:
    close(fd);
}


static int ipaddr_init(void)
{
	local_ipaddr_get();
	
	return 0;
}

fn_initcall_prio(ipaddr_init, 0);



static gboolean cancel_fire_3(gpointer data){
  g_print("cancel_fire_3() quit \n");
 
#if 0  
  GHashTableIter iter;
	gpointer key, value;
	
	g_hash_table_iter_init(&iter, info_htable);
	
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		pr_info("%s: key = %s, value = %s\n", __func__, key, value);
	}
	
	pr_info("%s: hwaddr  = %s\n", __func__, info_default_hwaddr_get());
	pr_info("%s: ipaddr  = %s\n", __func__, info_default_ipaddr_get());
	pr_info("%s: netmask = %s\n", __func__, info_default_netmask_get());
#else
	info_key_value_display();
#endif

  return G_SOURCE_REMOVE;
}

static int main_init(void)
{
	printf("-----main_init------ \n");

	g_timeout_add(1000, cancel_fire_3, NULL);
	
	return 0;
}
fn_initcall(main_init);

static gint compare_strings(gconstpointer a, gconstpointer b)
{
  char **str_a = (char **)a;
  char **str_b = (char **)b;

  return strcmp(*str_a, *str_b);
}

void info_key_value_display(void)
{
	GPtrArray *parr;
	GHashTableIter iter;
	gpointer key, value;
	
	parr = g_ptr_array_new(); 
	if (!parr) {
		pr_err("%s: <error> g_ptr_array_new\n", __func__);
		return;
	}
	g_hash_table_iter_init(&iter, info_htable);
	
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		g_ptr_array_add(parr, key);
		// pr_info("%s: key = %s, value = %s\n", __func__, key, value);
	}
	g_ptr_array_sort(parr, compare_strings);
	
	for(int i = 0; i < parr->len; i++) {
		key = g_ptr_array_index(parr, i);
		value = info_htable_lookup(key);
		pr_info("%s: %s\n", (char *)key, (char *)value);
    }
	g_ptr_array_free(parr, TRUE);
}

void info_htable_display(void)
{
	// main_init();
	info_key_value_display();
}




