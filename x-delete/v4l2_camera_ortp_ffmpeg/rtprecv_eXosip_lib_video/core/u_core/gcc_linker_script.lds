/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2011-2012 The Chromium OS Authors.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

SECTIONS
{
	.initcall : {
		/* KEEP(*(SORT(.data.initcall.*))); */
    	KEEP(*(.data.initcall.*))
    	SORT(CONSTRUCTORS)
  	}
}

/* INSERT AFTER .data; */
/* INSERT BEFORE .data; */
INSERT BEFORE .data;
/* reference from u-boot/arch/sandbox/cpu/u-boot-spl.lds */

