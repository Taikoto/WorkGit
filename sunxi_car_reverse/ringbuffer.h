/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#
typedef struct {
	unsigned int rb_capacity;
	char *rb_readtail;
	char *rb_writetail;
	char *rb_buff;
	unsigned rb_flag;
	struct mutex mutex;
} RingBuffer;

unsigned int rb_capacity(RingBuffer *rb);
unsigned int rb_can_read(RingBuffer *rb);
unsigned int rb_can_write(RingBuffer *rb);
unsigned int rb_new(RingBuffer *rb, int sz);
void rb_delete(RingBuffer *rb);
unsigned int rb_read(RingBuffer *rb, char *data, unsigned int count);
unsigned int rb_write(RingBuffer *rb, void *data, unsigned int count);
#endif
