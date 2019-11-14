/*
 *
 * btusb_proc.h
 *
 *
 *
 * Copyright (C) 2011-2014 Broadcom Corporation.
 *
 *
 *
 * This software is licensed under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation (the "GPL"), and may
 * be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL for more details.
 *
 *
 * A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php
 * or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA
 *
 * Module that defines the proc file system routines.
 *
 */

#ifndef BTUSB_PROC_H
#define BTUSB_PROC_H

struct btusb;

#ifdef CONFIG_PROC_FS

extern void btusb_proc_init(void);
extern void btusb_proc_exit(void);
extern void btusb_proc_add(struct btusb *p_dev, const char *name);
extern void btusb_proc_remove(struct btusb *p_dev, const char *name);

#else

static inline void btusb_proc_init(void) {}
static inline void btusb_proc_exit(void) {}
static inline void btusb_proc_add(struct btusb *p_dev, const char *name) {}
static inline void btusb_proc_remove(struct btusb *p_dev, const char *name) {}

#endif

#endif
