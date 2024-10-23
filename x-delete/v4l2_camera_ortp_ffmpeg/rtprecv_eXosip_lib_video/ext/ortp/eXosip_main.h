/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __EXOSIP_MAIN_H__
#define __EXOSIP_MAIN_H__
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <eXosip2/eXosip.h>
#include <osip2/osip_mt.h>

#include "initcall.h"


int eXosip_send_register_request(void);
int eXosip_send_unregister_request(void);
int eXosip_send_invite_request(void);
int eXosip_send_terminate_request(void);
int eXosip_send_information_request(void);
int eXosip_send_message_request(void);

bool exosip_eXosip_register(void);
bool exosip_eXosip_invite(void);
bool exosip_eXosip_message(void);
bool exosip_eXosip_terminate(void);
bool exosip_eXosip_unregister(void);

// exosip start event
bool exosip_call_ack_event(void);
bool exosip_call_answered_event(void);

// exosip finish event
bool exosip_call_closed_event(void);
bool exosip_call_message_answered_event(void);

#endif /* __EXOSIP_MAIN_H__ */


