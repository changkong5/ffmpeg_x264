/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __EMULPLAY_CAP_H__
#define __EMULPLAY_CAP_H__
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "initcall.h"

const char *dbus_service_name(void);

void emulplay_stop(void);
void emulplay_exit(void);
int emulplay_start(void);
int emulplay_write(void *data, int len);

void emulcap_stop(void);
void emulcap_exit(void);
int emulcap_start(void);
int emulcap_read(void *data, int len);


bool emulate_audio_play_start(void);
bool emulate_audio_play_stop(void);

bool emulate_audio_capture_start(void);
bool emulate_audio_capture_stop(void);

bool emulate_ortp_audio_start_i(void);
bool emulate_ortp_audio_stop_i(void);

#endif /* __EMULPLAY_CAP_H__ */


