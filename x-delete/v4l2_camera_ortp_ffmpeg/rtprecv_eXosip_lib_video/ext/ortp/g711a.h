/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __G711A_H__
#define __G711A_H__

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

uint8_t g711a_encode(int16_t pcm_val);
int16_t g711a_decode(uint8_t a_val);

#endif /* __G711A_H__ */


