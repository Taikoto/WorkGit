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


#include "ringbuffer.h"
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/unistd.h>

unsigned int rb_capacity(RingBuffer *rb)
{
	return rb->rb_capacity;
}

unsigned int rb_can_read(RingBuffer *rb)
{
	if ((rb->rb_readtail == rb->rb_writetail) && !(rb->rb_flag)) {
		return 0;
	}
	if (rb->rb_readtail < rb->rb_writetail)
		return rb->rb_writetail - rb->rb_readtail;
	else
		return (unsigned int)(rb_capacity(rb) - (rb->rb_readtail - rb->rb_writetail));
}

unsigned int rb_can_write(RingBuffer *rb)
{
	return rb_capacity(rb) - rb_can_read(rb);
}

unsigned int rb_new(RingBuffer *rb, int sz)
{
	if (!rb) {
		printk("rb is null(%d)\n", __LINE__);
		return 0;
	}
	rb->rb_capacity = sz;
	rb->rb_buff = (unsigned char *)kmalloc(sz, 0);
	if (rb->rb_buff == NULL) {
		printk("alloc buffer fail\n");
		return 0;
	}
	rb->rb_writetail = rb->rb_buff;
	rb->rb_readtail = rb->rb_buff;
	rb->rb_flag = 0;
	mutex_init(&rb->mutex);
	return 1;
};

void rb_delete(RingBuffer *rb)
{
	if (!rb || !rb->rb_buff) {
		printk("rb is null(%d)\n", __LINE__);
		return;
	};
	rb->rb_capacity = 0;
	kfree(rb->rb_buff);
	rb->rb_writetail = 0;
	rb->rb_readtail = 0;
	rb->rb_buff = 0;
	mutex_destroy(&rb->mutex);
	return;
};

unsigned int rb_read(RingBuffer *rb, char *data, unsigned int count)
{
	unsigned int leftsz = 0, copy_sz = 0, sz = 0;
	char *rb_writetail = 0;
	if (!rb || !rb->rb_buff) {
		printk("rb is null(%d)\n", __LINE__);
		return 0;
	}
	mutex_lock(&rb->mutex);
	if (!data || !rb->rb_flag) {
		mutex_unlock(&rb->mutex);
		return 0;
	}
	if (rb_can_read(rb) == 0) {
		mutex_unlock(&rb->mutex);
		return 0;
	}
	rb_writetail = rb->rb_writetail;
	mutex_unlock(&rb->mutex);
	if (rb->rb_readtail < rb_writetail) {
		if ((rb->rb_readtail + count) < rb_writetail) {
			copy_sz = count;
			memcpy(data, (char *)rb->rb_readtail, copy_sz);
			rb->rb_readtail += copy_sz;
			sz = count;
			goto end;
		} else {
			int copy_sz = rb_writetail - rb->rb_readtail;
			memcpy(data, (char *)rb->rb_readtail, copy_sz);
			rb->rb_readtail = rb_writetail;
			mutex_lock(&rb->mutex);
			rb->rb_flag = 0;
			mutex_unlock(&rb->mutex);
			sz = copy_sz;
			goto end;
		}
	} else {
		if (count <
		    (rb_capacity(rb) - (rb->rb_readtail - rb->rb_buff))) {
			copy_sz = count;
			memcpy(data, (char *)rb->rb_readtail, copy_sz);
			rb->rb_readtail += copy_sz;
			sz = count;
			goto end;
		} else {
			copy_sz =
			    rb_capacity(rb) - (rb->rb_readtail - rb->rb_buff);
			memcpy(data, (char *)rb->rb_readtail, copy_sz);
			rb->rb_readtail = rb->rb_buff;
			leftsz = count - copy_sz;
			if (leftsz < (rb_writetail - rb->rb_buff)) {
				memcpy(data, (char *)rb->rb_readtail, leftsz);
				rb->rb_readtail += leftsz;
				sz = count;
				goto end;
			} else {
				leftsz = rb_writetail - rb->rb_buff;
				memcpy(data, (char *)rb->rb_readtail, leftsz);
				rb->rb_readtail = rb_writetail;
				mutex_lock(&rb->mutex);
				rb->rb_flag = 0;
				mutex_unlock(&rb->mutex);
				sz = (leftsz + copy_sz);
				goto end;
			}
		}
	}
end:
	return sz;
}

unsigned int rb_write(RingBuffer *rb, void *data, unsigned int count)
{
	unsigned int copysz = 0, sz = 0;
	char *rb_readtail = 0;
	if (!rb || !rb->rb_buff) {
		printk("rb is null(%d)\n", __LINE__);
		return 0;
	}
	if (!data) {
		printk("data is null\n");
		return 0;
	}
	/*
	if (mutex_trylock(&rb->mutex) != 0)
		return 0;
	*/
	if (rb_can_write(rb) < count) {
		printk("xxx ringbuffer full(%d)\n", __LINE__);
		/*
			 mutex_unlock(&rb->mutex);
		*/
		return 0;
	}
	rb_readtail = rb->rb_readtail;

	if (rb_readtail <= rb->rb_writetail) {
		unsigned int tail_avail_sz =
		    rb_capacity(rb) - (rb->rb_writetail - rb->rb_buff);
		if (count <= tail_avail_sz) {
			memcpy(rb->rb_writetail, data, count);
			rb->rb_writetail += count;
			if (rb->rb_writetail == rb->rb_buff + rb_capacity(rb))
				rb->rb_writetail = rb->rb_buff;
			sz = count;
			goto end;
		} else {
			memcpy(rb->rb_writetail, data, tail_avail_sz);
			rb->rb_writetail = rb->rb_buff;
			if ((count - tail_avail_sz) <=
			    (unsigned int)(rb_readtail - rb->rb_buff)) {
				memcpy(rb->rb_buff, data + tail_avail_sz,
				       count - tail_avail_sz);
				rb->rb_writetail =
				    rb->rb_buff + (count - tail_avail_sz);
				sz = count;
				goto end;
			} else {
				copysz = rb_readtail - rb->rb_buff;
				memcpy(rb->rb_buff, data + tail_avail_sz,
				       copysz);
				rb->rb_writetail = rb_readtail;
				printk("ringbuffer full(%d)\n", __LINE__);
				sz = copysz + tail_avail_sz;
				goto end;
			}
			return 0;
		}

	} else {
		copysz = rb_readtail - rb->rb_writetail;
		if (copysz >= count) {
			memcpy(rb->rb_writetail, data, count);
			rb->rb_writetail += count;
			sz = count;
			goto end;
		} else {
			memcpy(rb->rb_writetail, data, copysz);
			rb->rb_writetail = rb_readtail;
			printk("ringbuffer full(%d)\n", __LINE__);
			sz = copysz;
			goto end;
		}
	}
end:
	rb->rb_flag = 1;
	/*
	mutex_unlock(&rb->mutex);
	*/
	return sz;
}
