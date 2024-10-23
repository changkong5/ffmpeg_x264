/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __TINYPLAY_CAP_H__
#define __TINYPLAY_CAP_H__
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "initcall.h"

const char *dbus_service_name(void);

void tinyplay_stop(void);
void tinyplay_exit(void);
int tinyplay_start(void);
int tinyplay_write(void *data, int len);

int tinyplay_remain_nbyte(void);
int tinyplay_set_nonblocking(int nonblock);
int tinyplay_set_rw_nonblocking(int nonblock);


void tinycap_stop(void);
void tinycap_exit(void);
int tinycap_start(void);
int tinycap_read(void *data, int len);

int tinycap_remain_nbyte(void);
int tinycap_set_nonblocking(int nonblock);
int tinycap_set_rw_nonblocking(int nonblock);

bool audio_play_start(void);
bool audio_play_stop(void);

bool audio_capture_start(void);
bool audio_capture_stop(void);

bool ortp_audio_start_i(void);
bool ortp_audio_stop_i(void);

#endif /* __TINYPLAY_CAP_H__ */


