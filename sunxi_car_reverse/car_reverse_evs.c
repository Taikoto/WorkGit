/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "upng.h"
#include "ringbuffer.h"
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/sunxi-gpio.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm_types.h> 
#include <linux/mm.h>
#include <linux/kernel.h>
#include "colormap.h"
#include "ubmp.h"
#include "evs_communication.h"
#include <linux/kprobes.h>
#include <asm/traps.h>
#include "mcu_data_misc.h"
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>


#define DEV_NAME "fast_car_reverse"
#define REVERSE_IOC_MAGIC  'z'

#define REVERSE_IOC_TEST_DATA  _IOW(REVERSE_IOC_MAGIC, 1, unsigned int *)
#define REVERSE_IOC_MCU_DATA  _IOW(REVERSE_IOC_MAGIC, 2, struct mcu_data_t *)
#define REVERSE_IOC_RMCU_DATA  _IOR(REVERSE_IOC_MAGIC, 3, struct mcu_data_t *)


struct mcu_data_t *mcu_data_prv;
struct semaphore mcu_wakesem;


//#include <linux/sys_config.h>
#ifdef USE_SERIAL
#define MCU_SERIAL_NUMBER 7
#endif
#ifdef CONFIG_SUNXI_DI
#define USE_SUNXI_DI_MODULE

#define ALIGN_16B(x) (((x) + (15)) & ~(15))

//#define PAGE_MASK   (~(PAGE_SIZE-1))
//#define ALLIGN_PAGE(addr)    (((addr)+PAGE_SIZE-1)&PAGE_MASK)
struct page *pngbuf = NULL;
//extern struct mcu_data_t *mcu_data_prv;
//extern struct semaphore mcu_wakesem;

enum __di_pixel_fmt_t {
	DI_FORMAT_NV12 = 0x00,    /* 2-plane */
	DI_FORMAT_NV21 = 0x01,    /* 2-plane */
	DI_FORMAT_MB32_12 = 0x02, /* NOT SUPPORTED, UV mapping like NV12 */
	DI_FORMAT_MB32_21 = 0x03, /* NOT SUPPORTED, UV mapping like NV21 */
	DI_FORMAT_YV12 = 0x04,    /* 3-plane */
	DI_FORMAT_YUV422_SP_UVUV = 0x08, /* 2-plane, New in DI_V2.2 */
	DI_FORMAT_YUV422_SP_VUVU = 0x09, /* 2-plane, New in DI_V2.2 */
	DI_FORMAT_YUV422P = 0x0c,	/* 3-plane, New in DI_V2.2 */
	DI_FORMAT_MAX,
};

enum __di_intp_mode_t {
	DI_MODE_WEAVE = 0x0, /* Copy source to destination */
	DI_MODE_INTP = 0x1, /* Use current field to interpolate another field */
	DI_MODE_MOTION = 0x2, /* Use 4-field to interpolate another field */
};

enum __di_updmode_t {
	DI_UPDMODE_FIELD = 0x0, /* Output 2 frames when updated 1 input frame */
	DI_UPDMODE_FRAME = 0x1, /* Output 1 frame when updated 1 input frame */
};

struct __di_rectsz_t {
	unsigned int width;
	unsigned int height;
};

struct __di_fb_t {
	void *addr[2];		   /* frame buffer address */
	struct __di_rectsz_t size; /* size pixel */
	enum __di_pixel_fmt_t format;
};

struct __di_para_t {
	struct __di_fb_t input_fb;	/* current frame fb */
	struct __di_fb_t pre_fb;	  /* previous frame fb */
	struct __di_rectsz_t source_regn; /* current frame and
					   * previous frame process region
					   */
	struct __di_fb_t output_fb;       /* output frame fb */
	struct __di_rectsz_t out_regn;    /* output frame region */
	__u32 field;			  /* process field <0-top field ;
					   * 1-bottom field>
					   */
	__u32 top_field_first;		  /* video infomation <0-is not
					   * top_field_first; 1-is top_
					   * field_first>
					   */
};

/* di_format_attr - display format attribute
 *
 * @format: pixel format
 * @bits: bits of each component
 * @hor_rsample_u: reciprocal of horizontal sample rate
 * @hor_rsample_v: reciprocal of horizontal sample rate
 * @ver_rsample_u: reciprocal of vertical sample rate
 * @hor_rsample_v: reciprocal of vertical sample rate
 * @uvc: 1: u & v component combined
 * @interleave: 0: progressive, 1: interleave
 * @factor & div: bytes of pixel = factor / div (bytes)
 *
 * @addr[out]: address for each plane
 * @trd_addr[out]: address for each plane of right eye buffer
 */
struct di_format_attr {
	enum __di_pixel_fmt_t format;
	unsigned int bits;
	unsigned int hor_rsample_u;
	unsigned int hor_rsample_v;
	unsigned int ver_rsample_u;
	unsigned int ver_rsample_v;
	unsigned int uvc;
	unsigned int interleave;
	unsigned int factor;
	unsigned int div;
};

struct __di_fb_t2 {
	int fd;
	unsigned long long addr[3]; /* frame buffer address */
	struct __di_rectsz_t size;  /* size (in pixel) */
	enum __di_pixel_fmt_t format;
};

struct __di_para_t2 {
	struct __di_fb_t2 input_fb;       /* current frame fb */
	struct __di_fb_t2 pre_fb;	 /* previous frame fb */
	struct __di_fb_t2 next_fb;	/* next frame fb */
	struct __di_rectsz_t source_regn; /* current frame /previous frame and
						next frame process region */
	struct __di_fb_t2 output_fb;      /* output frame fb */
	struct __di_rectsz_t out_regn;    /* output frame region */
	unsigned int field; /* process field <0-first field ; 1-second field> */
	unsigned int top_field_first; /* video infomation <0-is not
				top_field_first; 1-is top_field_first> */
	/* unsigned int update_mode; */
	/* update buffer mode <0-update 1 frame,
	output 2 frames; 1-update 1 frame, output 1 frame> */
	int id;
	int dma_if;
};

/* New in DI_2.X */
struct __di_mode_t {
	enum __di_intp_mode_t di_mode;
	enum __di_updmode_t update_mode;
};

struct __di_mem_t {
	unsigned int size;
	void *v_addr;
	unsigned long p_addr;
};

extern int draw_pic(int screen_id, int w, int h, char *buf);
extern int draw_image(void *base, unsigned int x, unsigned int y, unsigned int w,
	       unsigned int h, int stride_w, int stride_h, unsigned char *data,
	       unsigned int len);
extern int dec_png_fb(const char *path, void *buf, const int width, const int height);
extern unsigned int sunxi_di_request(void);
extern int sunxi_di_commit_special(struct __di_para_t2 *di_para, struct file *filp);
extern int sunxi_di_close(unsigned int id);
extern void sunxi_di_setmode(struct __di_mode_t *di_mode);

static int di_id;
static struct __di_mode_t car_dimode;

#else
#undef USE_SUNXI_DI_MODULE
#endif
static struct buffer_node *new_frame, *old_frame, *oold_frame;

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
#include <linux/switch.h>
#endif

#include "car_reverse.h"
#include "include.h"

#define MODULE_NAME "car-reverse"

#define SWAP_BUFFER_CNT (10)
#define SWAP_BUFFER_CNT_VIN (10)

#define THREAD_NEED_STOP (1 << 0)
#define THREAD_RUN (1 << 1)
#define CAR_REVSER_GPIO_LEVEL 1
#define USE_YUV422
/*#undef USE_YUV422*/
struct car_reverse_private_data {
	struct preview_params config;
	int reverse_gpio;
	#ifdef USE_SERIAL
	int reverse_can_used;
	int reverse_can_if;
	#endif

	struct buffer_pool *buffer_pool;
	struct buffer_pool *bufferOv_pool[CAR_MAX_CH];
	struct buffer_pool *bufferOv_preview[CAR_MAX_CH];
	struct buffer_node *buffer_disp[2];

	struct work_struct status_detect;
	struct workqueue_struct *preview_workqueue;

	struct task_struct *display_update_task;
	struct task_struct *display_frame_task;
	struct task_struct *display_view_task;
	#ifdef USE_SERIAL
	struct task_struct *serial_task;
	#else
	struct task_struct *mcu_data_task;
	#endif
	struct list_head pending_frame;
	struct list_head pending_frameOv[CAR_MAX_CH];
	spinlock_t display_lock;

	int needexit;
	int needfree;
	int status;
	int disp_index;
	int debug;
	int format;
	int used_oview;
	int thread_mask;
	int discard_frame;
	int ov_sync;
	int ov_sync_algo;
	int ov_sync_frame;
	int sync_w;
	int sync_r;
	int standby;
	int view_thread_start;
	int algo_thread_start;
	int view_mask;
	int algo_mask;
	spinlock_t thread_lock;
	spinlock_t free_lock;
};

static int rotate;

static char car_irq_pin_name[16];

module_param(rotate, int, 0644);

static struct car_reverse_private_data *car_reverse;

#define UPDATE_STATE 1
#if defined(UPDATE_STATE) &&                                                   \
    (defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH))

static ssize_t print_dev_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", sdev->name);
}

static struct switch_dev car_reverse_switch = {
    .name = "parking-switch", .state = 0, .print_name = print_dev_name,
};

static void car_reverse_switch_register(void)
{
	switch_dev_register(&car_reverse_switch);
}

static void car_reverse_switch_unregister(void)
{
	switch_dev_unregister(&car_reverse_switch);
}

static void car_reverse_switch_update(int flag)
{
	switch_set_state(&car_reverse_switch, flag);
}
#else
static void car_reverse_switch_register(void)
{
}
static void car_reverse_switch_update(int flag)
{
}

static void car_reverse_switch_unregister(void)
{
}

#endif

static void of_get_value_by_name(struct platform_device *pdev, const char *name,
				 int *retval, unsigned int defval)
{
	if (of_property_read_u32(pdev->dev.of_node, name, retval) != 0) {
		dev_err(&pdev->dev, "missing property '%s', default value %d\n",
			name, defval);
		*retval = defval;
	}
}

static void of_get_gpio_by_name(struct platform_device *pdev, const char *name,
				int *retval)
{
	int gpio_index;
	struct gpio_config config;

	gpio_index = of_get_named_gpio_flags(pdev->dev.of_node, name, 0,
					     (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(gpio_index)) {
		dev_err(&pdev->dev, "failed to get gpio '%s'\n", name);
		*retval = 0;
		return;
	}
	*retval = gpio_index;

	dev_info(&pdev->dev,
		 "%s: gpio=%d mul-sel=%d pull=%d drv_level=%d data=%d\n", name,
		 config.gpio, config.mul_sel, config.pull, config.drv_level,
		 config.data);
}

static void parse_config(struct platform_device *pdev,
			 struct car_reverse_private_data *priv)
{
	of_get_value_by_name(pdev, "tvd_id", &priv->config.tvd_id, 0);
	of_get_value_by_name(pdev, "screen_width", &priv->config.screen_width,
			     0);
	of_get_value_by_name(pdev, "screen_height", &priv->config.screen_height,
			     0);
	of_get_value_by_name(pdev, "rotation", &priv->config.rotation, 0);
	of_get_value_by_name(pdev, "source", &priv->config.input_src, 0);
	of_get_value_by_name(pdev, "oview", &priv->used_oview, 0);
	of_get_gpio_by_name(pdev, "reverse_pin", &priv->reverse_gpio);
	#ifdef USE_SERIAL
	of_get_value_by_name(pdev, "parse_can", &priv->reverse_can_used, 0);
	of_get_value_by_name(pdev, "can_if", &priv->reverse_can_if, 0);
	#endif
}

void car_reverse_display_update(int tvd_fd)
{
	int run_thread = 0;
	int n = 0;
	int tmp = 0;
	struct buffer_node *node;
	struct list_head *pending_frame = &car_reverse->pending_frame;

	spin_lock(&car_reverse->display_lock);

	if (car_reverse->used_oview) {
		tmp = car_reverse->sync_w - car_reverse->sync_r;
		if ((car_reverse->ov_sync & (1 << tvd_fd)) || tmp > 3) {
			for (n = 0; n < CAR_MAX_CH; n++) {
				if (car_reverse->config.input_src)
					node =
					    video_source_dequeue_buffer_vin(n);
				else
					node = video_source_dequeue_buffer(n);

				if (node) {
					if (car_reverse->config.input_src)
						video_source_queue_buffer_vin(
						    node, n);
					else
						video_source_queue_buffer(node,
									  n);
				}
			}
			car_reverse->ov_sync = 0;
		} else {
			car_reverse->ov_sync |= (1 << tvd_fd);
			if ((car_reverse->ov_sync & 0xf) == 0xf) {
				run_thread = 1;
				car_reverse->ov_sync = 0;
			}
		}
	}
	if (!car_reverse->used_oview) {
		while (!list_empty(pending_frame)) {
			node = list_entry(pending_frame->next,
					  struct buffer_node, list);
			list_del(&node->list);
			if (car_reverse->config.input_src)
				video_source_queue_buffer_vin(node, tvd_fd);
			else
				video_source_queue_buffer(node, tvd_fd);
		}
		if (car_reverse->config.input_src)
			node = video_source_dequeue_buffer_vin(tvd_fd);
		else
			node = video_source_dequeue_buffer(tvd_fd);
		if (node) {
			list_add(&node->list, pending_frame);
		}
	} else {
		if (run_thread) {
			for (n = 0; n < CAR_MAX_CH; n++) {
				pending_frame =
				    &car_reverse->pending_frameOv[n];
				if (car_reverse->config.input_src)
					node =
					    video_source_dequeue_buffer_vin(n);
				else
					node = video_source_dequeue_buffer(n);
				if (node) {
					list_add(&node->list, pending_frame);
				}
			}
		}
	}
	spin_unlock(&car_reverse->display_lock);

	if (car_reverse->used_oview) {
		if (run_thread) {
			if (car_reverse->display_update_task)
				wake_up_process(
				    car_reverse->display_update_task);
			car_reverse->thread_mask |= THREAD_RUN;
			car_reverse->sync_w++;
			run_thread = 0;
		}
	} else {

		spin_lock(&car_reverse->thread_lock);
		if (car_reverse->thread_mask & THREAD_NEED_STOP) {
			spin_unlock(&car_reverse->thread_lock);
			return;
		}
		if (car_reverse->display_update_task)
			wake_up_process(car_reverse->display_update_task);
		car_reverse->thread_mask |= THREAD_RUN;
		spin_unlock(&car_reverse->thread_lock);
	}
}

void car_do_freemem(struct work_struct *work)
{
	int i = 0;
	struct buffer_pool *bp = 0;
	spin_lock(&car_reverse->free_lock);
	if (car_reverse->needfree) {
		if (car_reverse->buffer_pool) {
			free_buffer_pool(car_reverse->config.dev,
					 car_reverse->buffer_pool);
			car_reverse->buffer_pool = 0;
		}
		for (i = 0; i < CAR_MAX_CH; i++) {
			bp = car_reverse->bufferOv_pool[i];
			if (bp) {
				free_buffer_pool(car_reverse->config.dev, bp);
				car_reverse->bufferOv_pool[i] = 0;
			}
			bp = car_reverse->bufferOv_preview[i];
			if (bp) {
				free_buffer_pool(car_reverse->config.dev, bp);
				car_reverse->bufferOv_preview[i] = 0;
			}
		}
		if (car_reverse->config.input_src == 0) {
			if (car_reverse->buffer_disp[0]) {
				__buffer_node_free(car_reverse->config.dev,
						   car_reverse->buffer_disp[0]);
				car_reverse->buffer_disp[0] = 0;
			}
			if (car_reverse->buffer_disp[1]) {
				__buffer_node_free(car_reverse->config.dev,
						   car_reverse->buffer_disp[1]);
				car_reverse->buffer_disp[1] = 0;
			}
		}
		logerror("car_reverse free buffer\n");
	} else
		logdebug("no need free buffer! \n");
	spin_unlock(&car_reverse->free_lock);
}

static DECLARE_DELAYED_WORK(car_freework, car_do_freemem);

int algo_frame_work(void *data)
{
	struct buffer_pool *bp = 0;
	struct buffer_pool *bp_preview = 0;
	int i = 0;
	int tmp = 0;
	int count = 0;
	struct buffer_node *new_frameOv[CAR_MAX_CH];
	while (!kthread_should_stop()) {
		car_reverse->algo_mask = 0;
		if (car_reverse->algo_thread_start) {
			tmp = car_reverse->ov_sync_frame -
			      car_reverse->ov_sync_algo;
			if (car_reverse->ov_sync_frame >
			    car_reverse->ov_sync_algo) {
				for (i = 0; i < CAR_MAX_CH; i++) {
					new_frameOv[i] = 0;
					bp = car_reverse->bufferOv_pool[i];
					if (bp)
						new_frameOv[i] =
						    bp->dequeue_buffer(bp);
				}
				if (tmp <= 4) {
					if (new_frameOv[0] && new_frameOv[1] &&
					    new_frameOv[2] && new_frameOv[3] &&
					    count) {
						preview_update_Ov(
						    new_frameOv,
						    car_reverse->config
							.car_direct,
						    car_reverse->config
							.lr_direct);
					}

					if (count >= 3)
						count = 0;
					else
						count++;

					for (i = 0; i < CAR_MAX_CH; i++) {
						bp_preview =
						    car_reverse
							->bufferOv_preview[i];
						if (new_frameOv[i]) {
							if (car_reverse
								->display_view_task)
								bp_preview
								    ->queue_buffer(
									bp_preview,
									new_frameOv
									    [i]);
							else {
								video_source_queue_buffer_vin(
								    new_frameOv
									[i],
								    i);
							}
						}
					}
					car_reverse->ov_sync_algo++;
				} else {
					while (tmp > 0 &&
					       !kthread_should_stop()) {

						for (i = 0; i < CAR_MAX_CH;
						     i++) {
							new_frameOv[i] = 0;
							bp = car_reverse
								 ->bufferOv_pool
								     [i];
							if (bp)
								new_frameOv[i] =
								    bp->dequeue_buffer(
									bp);
							if (new_frameOv[i]) {
								video_source_queue_buffer_vin(
								    new_frameOv
									[i],
								    i);
							}
						}
						car_reverse->ov_sync_algo++;
						tmp =
						    car_reverse->ov_sync_frame -
						    car_reverse->ov_sync_algo;
						schedule_timeout(HZ / 10000);
					}
				}
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			set_current_state(TASK_RUNNING);
		if (!car_reverse->algo_thread_start) {
			car_reverse->algo_mask = 1;
			schedule();
		} else
			schedule_timeout(HZ / 10000);
	}
	return 0;
}

int display_view_work(void *data)
{
	struct buffer_pool *bp_preview = 0;
	struct buffer_node *frameOv[4];
	int i = 0;
	while (!kthread_should_stop()) {
		if (car_reverse->view_thread_start) {
			car_reverse->view_mask = 0;
			display_frame_work();
			for (i = 0; i < CAR_MAX_CH; i++) {
				bp_preview = car_reverse->bufferOv_preview[i];
				frameOv[i] =
				    bp_preview->dequeue_buffer(bp_preview);
				if (frameOv[i]) {
					video_source_queue_buffer_vin(
					    frameOv[i], i);
				}
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			set_current_state(TASK_RUNNING);
		if (!car_reverse->view_thread_start) {
			car_reverse->view_mask = 1;
			schedule();
		} else
			schedule_timeout(HZ / 10000);
	}
	return 0;
}

#ifdef USE_SERIAL
extern int serial_setup_special(int index, char *options);
extern void serial_close_special(void);
extern void close_special_serial(void);
extern void serial_write_special(int index, const char *s, unsigned int count);
extern void serial_register_buffer_done_rx(void *func);
extern void Cobs_Encrypt(const unsigned char *ptr, unsigned long length,unsigned char *dst);
extern void Cobs_Decrypt(const unsigned char *ptr, unsigned long length,unsigned char *dst);


RingBuffer car_serial_buf;

void serial_rx_callback(unsigned char *buf, int len)
{
	rb_write(&car_serial_buf, buf, len);
	return;
}

int car_status_flag = 0;
int car_reverse_status_flag = 0;

void parse_can_info(unsigned char *buf, int len)
{
    unsigned char realbuf[60];
	unsigned char ret = 0;
	int i = 0;
	printk("len = %d\r\n",len);
    Cobs_Decrypt(buf,len,realbuf);
	
	#if 0
	for(i = 0; i < len; i++) {
    	printk("buf[%d]=%3x ",i,realbuf[i]);
		if(i == len) {
        	printk("\n");
		}
	}
	#endif
	
	car_status_flag = get_smd_state(realbuf);
	ret = parse_data_info(car_status_flag, realbuf);
	printk("ret = 0x%x\r\n",ret);
	car_reverse_status_flag = car_reverse_status(realbuf);

	switch (car_reverse_status_flag) {
	case 1:
		car_reverse->debug = CAR_REVERSE_START;
        queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
		break;
	case 2:
		car_reverse->debug = CAR_REVERSE_STOP;
        queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
		break;
	case 0:
	default:
		break;
	}
	
    printk("%s car_status_flag = %d\r\n",__FUNCTION__, car_status_flag);
	printk("%s Exit !\r\n",__FUNCTION__);
	return;
}

int serial_parse_work(void *data)
{
	char serial_rx[20];
	unsigned int ret = 0;
	unsigned int len;
	unsigned int can_if = 0;

	int screen_id = 0;
	int retry = 0;
	
	rb_new(&car_serial_buf, 4096);
	can_if = car_reverse->reverse_can_if;
	if (can_if) {
		can_if = MCU_SERIAL_NUMBER;
	}
	serial_setup_special(can_if, "115200");
	serial_register_buffer_done_rx(serial_rx_callback);
	//printk("rb_buff %s\r\n",car_serial_buf.rb_buff);
	serial_write_special(can_if, "AW serial test\n",
			     strlen("AW serial test\n"));
	serial_write_special(can_if, "00 00",strlen("00 00"));
	printk("serial_parse_work while\r\n");
    //printk("rb_buff %s\r\n",car_serial_buf.rb_buff);
	while (!kthread_should_stop()) {
		len = rb_can_read(&car_serial_buf);
		//printk("serial_parse_work while %d\r\n",len);
		if(car_reverse->needexit == 1 && retry == 0) {
        	serial_close_special();
	        rb_delete(&car_serial_buf);
		    retry = 1;
			printk("serial_close_special retry = %d\r\n",retry);
		}
		
		if (len >= 2) {
			memset(serial_rx, 0, sizeof(serial_rx));
			rb_read(&car_serial_buf, serial_rx, len);
			//printk("serial_rx = %s\r\n", serial_rx);
			parse_can_info(serial_rx, len);
		}

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop()) {
			printk("kthread_should_stop\r\n");
			set_current_state(TASK_RUNNING);
			//set_current_state(TASK_STOPPED);
		}
		schedule_timeout(HZ / 10000);
	}
	serial_close_special();
	rb_delete(&car_serial_buf);

	printk("%s Exit !\r\n",__FUNCTION__);
	return 0;
}
#else
int car_status_flag = 0;
int ioctl_flag = 0;
int car_reverse_status_flag = 0;
int data_buf[6] = {0};

void get_mcu_data(void)
{
   int ret = 0;
   
   printk("get_mcu_data this is user(mcu) data:  11111\r\n");
   printk("smd_info=%d\n",mcu_data_prv->smd_info);
   printk("actual_gear=%d\n",mcu_data_prv->actual_gear);
   printk("steering_angle=%d\n",mcu_data_prv->steering_angle);
   printk("r_radar_distance=%d\n",mcu_data_prv->r_radar_distance);
   printk("m_radar_distance=%d\n",mcu_data_prv->m_radar_distance);
   printk("l_radar_distance=%d\n",mcu_data_prv->l_radar_distance);

   car_status_flag = get_smd_state(data_buf);
   printk("car_status_flag = %d\r\n",car_status_flag);
   ret = parse_data_info(car_status_flag, data_buf);
   printk("ret = %d\r\n",ret);
   //car_reverse_status_flag = car_reverse_status(realbuf);
   switch (mcu_data_prv->actual_gear) {
   case 2:
	   car_reverse->debug = CAR_REVERSE_START;
	   queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	   break;
   case 1:
	   car_reverse->debug = CAR_REVERSE_STOP;
	   queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	   break;
   case 0:
   default:
	   break;
   }
   
   printk("%s Exit\r\n",__FUNCTION__);
}

int mcu_data_work(void *data)
{
    int ret = 0;
	
	printk("1###################&&&&&&&&&& down mcu_wakesem\r\n");
	while (!kthread_should_stop()) {
		//len = rb_can_read(&car_serial_buf);
		//printk("mcu_data_work while %d\r\n",len);
		
		//down(&mcu_wakesem);
		//printk("2###################&&&&&&&&&& down mcu_wakesem\r\n");
	    if(ioctl_flag) {
			get_mcu_data();
			ioctl_flag = 0;
	    }

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop()) {
			printk("kthread_should_stop\r\n");
			set_current_state(TASK_RUNNING);
			//set_current_state(TASK_STOPPED);
		}
		schedule_timeout(HZ / 10000);
	}

	printk("%s Exit !\r\n",__FUNCTION__);
	return 0;
}
#endif
static int display_update_thread(void *data)
{
	struct list_head *pending_frame = &car_reverse->pending_frame;
	struct buffer_pool *bp = car_reverse->buffer_pool;
	int ret = 0;
	int i = 0;
	int firstframe = 0;
	struct buffer_node *new_frameOv[4];
#ifdef USE_SUNXI_DI_MODULE
	struct file car_file;
	unsigned int src_width, src_height;
	unsigned int dst_width, dst_height;
	struct __di_para_t2 di_para;
#endif
	new_frame = NULL;
	old_frame = NULL;
	oold_frame = NULL;
	while (!kthread_should_stop()) {
		bp = car_reverse->buffer_pool;
		if (!car_reverse->used_oview) {
			ret = spin_is_locked(&car_reverse->display_lock);
			if (ret) {
				goto disp_loop;
			}
		}

		if (!car_reverse->used_oview)
			spin_lock(&car_reverse->display_lock);

		if (car_reverse->config.input_src && car_reverse->used_oview) {
			if (car_reverse->sync_w != car_reverse->sync_r) {
				car_reverse->sync_r++;
				for (i = 0; i < CAR_MAX_CH; i++) {
					new_frameOv[i] = NULL;
					pending_frame =
					    &car_reverse->pending_frameOv[i];
					bp = car_reverse->bufferOv_pool[i];
					if (pending_frame->next !=
					    pending_frame) {
						new_frameOv[i] = list_entry(
						    pending_frame->next,
						    struct buffer_node, list);
						list_del(&new_frameOv[i]->list);
						bp->queue_buffer(
						    bp, new_frameOv[i]);
					}
				}
				car_reverse->ov_sync_frame++;
			}
		} else {
			if (pending_frame->next != pending_frame) {
				new_frame =
				    list_entry(pending_frame->next,
					       struct buffer_node, list);
				list_del(&new_frame->list);
				bp->queue_buffer(bp, new_frame);
			}
		}
#ifdef USE_SUNXI_DI_MODULE
		if (car_reverse->config.input_src) {
			if (!car_reverse->used_oview) {
				old_frame = bp->dequeue_buffer(bp);
				list_add(&old_frame->list, pending_frame);
				spin_unlock(&car_reverse->display_lock);
				preview_update(new_frame,
					       car_reverse->config.car_direct,
					       car_reverse->config.lr_direct);
			}
		} else {
			if (!firstframe) {
				firstframe = 1;
			}
			if (old_frame) {
				oold_frame = old_frame;
				old_frame = bp->dequeue_buffer(bp);
			} else {
				old_frame = bp->dequeue_buffer(bp);
			}
			spin_unlock(&car_reverse->display_lock);
			if (oold_frame && old_frame && new_frame) {
				src_height = car_reverse->config.src_height;
				src_width = car_reverse->config.src_width;
				dst_width = src_width;
				dst_height = src_height;
				di_para.pre_fb.addr[0] =
				    *(unsigned long long *)(&(
					oold_frame->phy_address));
				di_para.pre_fb.addr[1] =
				    *(unsigned long long *)(&(
					oold_frame->phy_address)) +
				    ALIGN_16B(src_width) * src_height;
				di_para.pre_fb.addr[2] = 0x0;
				di_para.pre_fb.size.width = src_width;
				di_para.pre_fb.size.height = src_height;
				if (car_reverse->config.format ==
				    V4L2_PIX_FMT_NV61)
					di_para.pre_fb.format =
					    DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.pre_fb.format = DI_FORMAT_NV21;

				di_para.input_fb.addr[0] =
				    *(unsigned long long *)(&(
					old_frame->phy_address));
				di_para.input_fb.addr[1] =
				    *(unsigned long long *)(&(
					old_frame->phy_address)) +
				    ALIGN_16B(src_width) * src_height;
				di_para.input_fb.addr[2] = 0x0;
				di_para.input_fb.size.width = src_width;
				di_para.input_fb.size.height = src_height;
				if (car_reverse->config.format ==
				    V4L2_PIX_FMT_NV61)
					di_para.input_fb.format =
					    DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.input_fb.format =
					    DI_FORMAT_NV21;

				di_para.next_fb.addr[0] =
				    *(unsigned long long *)(&(
					new_frame->phy_address));
				di_para.next_fb.addr[1] =
				    *(unsigned long long *)(&(
					new_frame->phy_address)) +
				    ALIGN_16B(src_width) * src_height;
				di_para.next_fb.addr[2] = 0x0;
				di_para.next_fb.size.width = src_width;
				di_para.next_fb.size.height = src_height;
				if (car_reverse->config.format ==
				    V4L2_PIX_FMT_NV61)
					di_para.next_fb.format =
					    DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.next_fb.format = DI_FORMAT_NV21;

				di_para.source_regn.width = src_width;
				di_para.source_regn.height = src_height;

				di_para.output_fb.addr[0] = *(
				    unsigned long long *)(&(
				    car_reverse
					->buffer_disp[car_reverse->disp_index]
					->phy_address));
				di_para.output_fb.addr[1] =
				    *(unsigned long long *)(&(
					car_reverse
					    ->buffer_disp[car_reverse
							      ->disp_index]
					    ->phy_address)) +
				    ALIGN_16B(dst_width) * dst_height;
				di_para.output_fb.addr[2] = 0x0;
				di_para.output_fb.size.width = dst_width;
				di_para.output_fb.size.height = dst_height;
				if (car_reverse->config.format ==
				    V4L2_PIX_FMT_NV61)
					di_para.output_fb.format =
					    DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.output_fb.format =
					    DI_FORMAT_NV21;

				di_para.out_regn.width = dst_width;
				di_para.out_regn.height = dst_height;

				di_para.field = 0;
				di_para.top_field_first = 1;
				di_para.id = di_id;
				di_para.dma_if = 1;
				sunxi_di_commit_special(&di_para, &car_file);
				di_para.field = 1;
				di_para.top_field_first = 1;
				di_para.id = di_id;
				di_para.dma_if = 1;
				sunxi_di_commit_special(&di_para, &car_file);
				if (oold_frame) {
					spin_lock(&car_reverse->display_lock);
					list_add(&oold_frame->list,
						 pending_frame);
					spin_unlock(&car_reverse->display_lock);
				}
				if (car_reverse->discard_frame != 0) {
					car_reverse->discard_frame--;
				} else
					car_reverse->discard_frame = 0;
				if (car_reverse->discard_frame == 0) {
					preview_update(
					    car_reverse->buffer_disp
						[car_reverse->disp_index],
					    car_reverse->config.car_direct,
					    car_reverse->config.lr_direct);
				}
				if (car_reverse->disp_index)
					car_reverse->disp_index = 0;
				else
					car_reverse->disp_index = 1;

			} else {
				preview_update(new_frame,
					       car_reverse->config.car_direct,
					       car_reverse->config.lr_direct);
			}
		}
#else
		old_frame = bp->dequeue_buffer(bp);
		list_add(&old_frame->list, pending_frame);
		spin_unlock(&car_reverse->display_lock);
		if (!firstframe) {
			printk(KERN_ERR "frame +++\n");
			firstframe = 1;
		}
		preview_update(new_frame, car_reverse->config.car_direct,
			       car_reverse->config.lr_direct);
#endif

	disp_loop:

		car_reverse->thread_mask &= (~THREAD_RUN);
		if (car_reverse->config.input_src && car_reverse->used_oview) {
			if (car_reverse->sync_w == car_reverse->sync_r)
				schedule();
			else
				schedule_timeout(HZ / 10000);
			if (kthread_should_stop()) {
				break;
			}
		} else {
			set_current_state(TASK_INTERRUPTIBLE);
			if (kthread_should_stop()) {
				break;
			}
			schedule();
		}
	}
	logerror("%s stop\n", __func__);
	return 0;
}

static int car_reverse_preview_start(void)
{
	int retval = 0;
	int i, n;
	struct buffer_node *node;
	struct buffer_pool *bp = 0;
	unsigned int buf_cnt = 0;
	car_reverse->disp_index = 0;
	car_reverse->discard_frame = 10;
	car_reverse->needfree = 0;
	car_reverse->config.car_oview_mode = car_reverse->used_oview;

	printk("in allen car_reverse_preview_start---\n");
	cancel_delayed_work(&car_freework);
	car_reverse->display_update_task =
	    kthread_create(display_update_thread, NULL, "sunxi-preview");
	if (!car_reverse->display_update_task) {
		printk(KERN_ERR "failed to create kthread\n");
		return -1;
	}
	/* FIXME: Calculate buffer size by preview info */
	//spin_lock(&car_reverse->free_lock);
	printk("car_reverse->config.input_src = %d\r\n",car_reverse->config.input_src);
	if (car_reverse->config.input_src) {
		if (car_reverse->buffer_pool == 0 && !car_reverse->used_oview) {
			car_reverse->buffer_pool = alloc_buffer_pool(
			    car_reverse->config.dev, SWAP_BUFFER_CNT_VIN,
			    1280 * 720 * 2);
		}
		if (car_reverse->used_oview) {
			for (i = 0; i < CAR_MAX_CH; i++) {
				if (car_reverse->bufferOv_pool[i] == 0 &&
				    car_reverse->used_oview) {
					car_reverse->bufferOv_pool[i] =
					    alloc_buffer_pool(
						car_reverse->config.dev,
						SWAP_BUFFER_CNT_VIN,
						1280 * 720 * 2);
				}

				if (car_reverse->bufferOv_preview[i] == 0 &&
				    car_reverse->used_oview) {
					car_reverse->bufferOv_preview[i] =
					    alloc_buffer_pool(
						car_reverse->config.dev,
						SWAP_BUFFER_CNT_VIN, 0);
				}
			}
		}
		car_reverse->buffer_disp[0] = 0;
		car_reverse->buffer_disp[1] = 0;
		buf_cnt = SWAP_BUFFER_CNT_VIN;

	printk("!car_reverse->display_update_task\r\n");
	} else {
		if (car_reverse->buffer_pool == 0)
			car_reverse->buffer_pool =
			    alloc_buffer_pool(car_reverse->config.dev,
					      SWAP_BUFFER_CNT, 720 * 576 * 2);
		if (car_reverse->buffer_disp[0] == 0)
			car_reverse->buffer_disp[0] = __buffer_node_alloc(
			    car_reverse->config.dev, 720 * 576 * 2, 0);
		memset(car_reverse->buffer_disp[0]
						   ->vir_address,
					       0x10,
					       720 * 576);
		memset(car_reverse->buffer_disp[0]->vir_address +
						   720 * 576,
					       0x80,
					       720 * 576);
		if (car_reverse->buffer_disp[1] == 0)
			car_reverse->buffer_disp[1] = __buffer_node_alloc(
			    car_reverse->config.dev, 720 * 576 * 2, 0);
		memset(car_reverse->buffer_disp[1]
						   ->vir_address,
					       0x10,
					       720 * 576);
		memset(car_reverse->buffer_disp[1]->vir_address +
						   720 * 576,
					       0x80,
					       720 * 576);
		buf_cnt = SWAP_BUFFER_CNT;
	}
	//spin_unlock(&car_reverse->free_lock);
	if (!car_reverse->buffer_pool && !car_reverse->used_oview) {
		dev_err(car_reverse->config.dev,
			"alloc buffer memory failed\n");
		goto gc;
	}
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		if (!car_reverse->bufferOv_pool[0] ||
		    !car_reverse->bufferOv_pool[1] ||
		    !car_reverse->bufferOv_pool[2] ||
		    !car_reverse->bufferOv_pool[3]) {
			dev_err(car_reverse->config.dev,
				"alloc buffer memory oview failed\n");
			goto gc;
		}

		if (!car_reverse->bufferOv_preview[0] ||
		    !car_reverse->bufferOv_preview[1] ||
		    !car_reverse->bufferOv_preview[2] ||
		    !car_reverse->bufferOv_preview[3]) {
			dev_err(car_reverse->config.dev,
				"alloc buffer memory oview failed\n");
			goto gc;
		}
	}

	if (car_reverse->config.input_src == 0) {
		if (!car_reverse->buffer_disp[0] ||
		    !car_reverse->buffer_disp[1]) {
			dev_err(car_reverse->config.dev,
				"alloc buffer memory failed\n");
			goto gc;
		}
	}
	if (car_reverse->config.input_src) {
		car_reverse->config.format = V4L2_PIX_FMT_NV21;
	} else {
		if (car_reverse->format)
			car_reverse->config.format = V4L2_PIX_FMT_NV61;
		else
			car_reverse->config.format = V4L2_PIX_FMT_NV21;
	}

	if (car_reverse->used_oview && car_reverse->config.input_src) {

		for (n = 0; n < CAR_MAX_CH; n++) {
			bp = car_reverse->bufferOv_pool[n];

			INIT_LIST_HEAD(&car_reverse->pending_frameOv[n]);
			retval = video_source_connect(&car_reverse->config, n);
			if (retval != 0) {
				logerror("can't connect to video source!\n");
				goto gc;
			}

			for (i = 0; i < buf_cnt; i++) {
				node = bp->dequeue_buffer(bp);
				if (car_reverse->config.input_src)
					video_source_queue_buffer_vin(node, n);
				else
					video_source_queue_buffer(node, n);
			}
		}
		car_reverse->ov_sync_frame = 0;
		car_reverse->ov_sync_algo = 0;
		car_reverse->sync_r = 0;
		car_reverse->sync_w = 0;

		car_reverse->display_frame_task =
		    kthread_create(algo_frame_work, NULL, "algo-preview");
		if (!car_reverse->display_frame_task) {
			printk(KERN_ERR "failed to create kthread\n");
			goto gc;
		}

		printk(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
#if 0
		car_reverse->display_view_task =
		    kthread_create(display_view_work, NULL, "view-preview");
		if (!car_reverse->display_view_task) {
			printk(KERN_ERR "failed to create kthread\n");
			goto gc;
		}
#endif
		if (car_reverse->display_frame_task) {
			struct sched_param param = {.sched_priority =
							MAX_RT_PRIO - 1};
			set_user_nice(car_reverse->display_frame_task, -20);
			sched_setscheduler(car_reverse->display_frame_task,
					   SCHED_FIFO, &param);
		}
		if (car_reverse->display_view_task) {
			struct sched_param param = {.sched_priority =
							MAX_RT_PRIO - 1};
			set_user_nice(car_reverse->display_view_task, -20);
			sched_setscheduler(car_reverse->display_view_task,
					   SCHED_FIFO, &param);
			car_reverse->config.viewthread = 1;

		} else {
			car_reverse->config.viewthread = 0;
		}
		car_reverse->view_thread_start = 1;
		car_reverse->algo_thread_start = 1;
		msleep(1);
		if (car_reverse->display_frame_task)
			wake_up_process(car_reverse->display_frame_task);
		if (car_reverse->display_view_task)
			wake_up_process(car_reverse->display_view_task);
		printk("in allen before preview_output_start\n");
		preview_output_start(&car_reverse->config);
		for (n = 0; n < CAR_MAX_CH; n++) {
			video_source_streamon_vin(n);
		}

	} else {
		bp = car_reverse->buffer_pool;

		INIT_LIST_HEAD(&car_reverse->pending_frame);
		retval = video_source_connect(&car_reverse->config,
					      car_reverse->config.tvd_id);
		if (retval != 0) {
			logerror("can't connect to video source!\n");
			goto gc;
		}

		preview_output_start(&car_reverse->config);
		//preview_update(car_reverse->buffer_disp[0], car_reverse->config.car_direct,
		//				   car_reverse->config.lr_direct);

		for (i = 0; i < buf_cnt; i++) {
			node = bp->dequeue_buffer(bp);
			if (car_reverse->config.input_src)
				video_source_queue_buffer_vin(
				    node, car_reverse->config.tvd_id);
			else
				video_source_queue_buffer(
				    node, car_reverse->config.tvd_id);
		}
		if (car_reverse->config.input_src)
			video_source_streamon_vin(car_reverse->config.tvd_id);
		else
			video_source_streamon(car_reverse->config.tvd_id);

	}
	car_reverse->status = CAR_REVERSE_START;
	car_reverse->thread_mask = 0;
#ifdef USE_SUNXI_DI_MODULE
	di_id = sunxi_di_request();
	car_dimode.di_mode = DI_MODE_MOTION;
	car_dimode.update_mode = DI_UPDMODE_FIELD;
	sunxi_di_setmode(&car_dimode);
#endif

	return 0;
gc:
	car_reverse->needfree = 1;
	schedule_delayed_work(&car_freework, 2 * HZ);
	return -1;
}

static int car_reverse_preview_stop(void)
{
	struct buffer_node *node;
	int i;
	struct buffer_pool *bp = car_reverse->buffer_pool;
	struct buffer_pool *preview_bp = 0;
	struct list_head *pending_frame = &car_reverse->pending_frame;
	printk(KERN_ERR "car_reverse_preview_stop start\n");

	car_reverse->status = CAR_REVERSE_STOP;
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		for (i = 0; i < CAR_MAX_CH; i++)
			video_source_streamoff_vin(i);
	} else {
		if (car_reverse->config.input_src)
			video_source_streamoff_vin(car_reverse->config.tvd_id);
		else
			video_source_streamoff(car_reverse->config.tvd_id);
	}
	spin_lock(&car_reverse->thread_lock);
	car_reverse->thread_mask |= THREAD_NEED_STOP;
	spin_unlock(&car_reverse->thread_lock);
	#ifdef USE_SERIAL
	printk("car_reverse->serial_task = %d\r\n",car_reverse->serial_task);
	if (car_reverse->serial_task) {
		printk("stop serial task thread\r\n");
		kthread_stop(car_reverse->serial_task);
	}
	car_reverse->serial_task = 0;
	#else
	//printk("car_reverse->mcu_data_task = %d\r\n",car_reverse->mcu_data_task);
	//if (car_reverse->mcu_data_task) {
		//printk("stop mcu data task thread\r\n");
		//kthread_stop(car_reverse->mcu_data_task);
	//}
	//car_reverse->mcu_data_task = 0;	
	#endif
	
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		struct sched_param param = {.sched_priority = MAX_RT_PRIO - 40};
		set_user_nice(car_reverse->display_frame_task, 0);
		sched_setscheduler(car_reverse->display_frame_task,
				   SCHED_NORMAL, &param);
		car_reverse->view_thread_start = 0;
		car_reverse->algo_thread_start = 0;
		car_reverse->ov_sync_frame = 0;
		car_reverse->ov_sync_algo = 0;
		car_reverse->sync_r = 0;
		car_reverse->sync_w = 0;
		msleep(10);
		while (car_reverse->thread_mask & THREAD_RUN)
			msleep(1);
		while (!car_reverse->algo_mask &&
		       car_reverse->display_frame_task)
			msleep(1);
		while (!car_reverse->view_mask &&
		       car_reverse->display_view_task)
			msleep(1);
		if (car_reverse->display_frame_task)
			kthread_stop(car_reverse->display_frame_task);
		if (car_reverse->display_view_task)
			kthread_stop(car_reverse->display_view_task);
		while (car_reverse->thread_mask & THREAD_RUN)
			msleep(1);
		kthread_stop(car_reverse->display_update_task);
		car_reverse->display_frame_task = 0;
		car_reverse->display_view_task = 0;
		car_reverse->display_update_task = 0;
	} else {
		while (car_reverse->thread_mask & THREAD_RUN)
			msleep(1);
		kthread_stop(car_reverse->display_update_task);
		car_reverse->display_update_task = 0;
	}

	preview_output_stop(&car_reverse->config);

__buffer_gc:
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		for (i = 0; i < CAR_MAX_CH; i++) {
			bp = car_reverse->bufferOv_pool[i];
			while (1) {
				node = video_source_dequeue_buffer_vin(i);
				if (node) {
					bp->queue_buffer(bp, node);
					logdebug("%s: collect %p\n", __func__,
						 node->phy_address);

				} else {
					video_source_disconnect(
					    &car_reverse->config, i);
					break;
				}
			}
		}
		for (i = 0; i < CAR_MAX_CH; i++) {
			bp = car_reverse->bufferOv_pool[i];
			preview_bp = car_reverse->bufferOv_preview[i];
			if (preview_bp) {
				while (1) {
					node = preview_bp->dequeue_buffer(
					    preview_bp);
					if (node) {
						bp->queue_buffer(bp, node);
						logdebug("%s: collect %p\n",
							 __func__,
							 node->phy_address);

					} else {
						break;
					}
				}
				rest_buffer_pool(NULL, preview_bp);
				dump_buffer_pool(NULL, preview_bp);
			}
		}

		for (i = 0; i < CAR_MAX_CH; i++) {
			spin_lock(&car_reverse->display_lock);
			pending_frame = &car_reverse->pending_frameOv[i];
			bp = car_reverse->bufferOv_pool[i];
			while (!list_empty(pending_frame)) {
				node = list_entry(pending_frame->next,
						  struct buffer_node, list);
				list_del(&node->list);
				bp->queue_buffer(bp, node);
			}
			spin_unlock(&car_reverse->display_lock);
			rest_buffer_pool(NULL, bp);
			dump_buffer_pool(NULL, bp);
		}

	} else {
		if (car_reverse->config.input_src)
			node = video_source_dequeue_buffer_vin(
			    car_reverse->config.tvd_id);
		else
			node = video_source_dequeue_buffer(
			    car_reverse->config.tvd_id);
		if (node) {
			bp->queue_buffer(bp, node);
			logdebug("%s: collect %p\n", __func__,
				 node->phy_address);
			goto __buffer_gc;
		}

		spin_lock(&car_reverse->display_lock);
		while (!list_empty(pending_frame)) {
			node = list_entry(pending_frame->next,
					  struct buffer_node, list);
			list_del(&node->list);
			bp->queue_buffer(bp, node);
		}
		spin_unlock(&car_reverse->display_lock);
		rest_buffer_pool(NULL, bp);
		dump_buffer_pool(NULL, bp);

		video_source_disconnect(&car_reverse->config,
					car_reverse->config.tvd_id);
	}
#ifdef USE_SUNXI_DI_MODULE
	sunxi_di_close(di_id);
	di_id = -1;
#endif
	/*
		if(car_reverse->buffer_pool) {
			free_buffer_pool(car_reverse->config.dev,
	   car_reverse->buffer_pool);
			car_reverse->buffer_pool = 0;
		}
		if (car_reverse->config.input_src == 0) {
			if(car_reverse->buffer_disp[0]) {
				__buffer_node_free(car_reverse->config.dev,
						car_reverse->buffer_disp[0]);
				car_reverse->buffer_disp[0] = 0;
			}
			if(car_reverse->buffer_disp[1]) {
				__buffer_node_free(car_reverse->config.dev,
						car_reverse->buffer_disp[1]);
				car_reverse->buffer_disp[1] = 0;
			}
		}
	*/
	car_reverse->needfree = 1;
	schedule_delayed_work(&car_freework, 2 * HZ);

	new_frame = NULL;
	old_frame = NULL;
	oold_frame = NULL;
	printk(KERN_ERR "car_reverse_preview_stop finish\n");
	return 0;
}

void car_reverse_set_int_ioin(void)
{
	long unsigned int config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, car_irq_pin_name, &config);
	if (0 != SUNXI_PINCFG_UNPACK_VALUE(config)) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0);
		pin_config_set(SUNXI_PINCTRL, car_irq_pin_name, config);
	}
}

void car_reverse_set_io_int(void)
{
	long unsigned int config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, car_irq_pin_name, &config);
	if (6 != SUNXI_PINCFG_UNPACK_VALUE(config)) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 6);
		pin_config_set(SUNXI_PINCTRL, car_irq_pin_name, config);
	}
}

static int car_reverse_gpio_status(void)
{
#ifdef _REVERSE_DEBUG_
	printk("in allen car_reverse_gpio_status 1111, car_reverse->debug = %d\n",car_reverse->debug);
	return (car_reverse->debug == CAR_REVERSE_START ? CAR_REVERSE_START
							: CAR_REVERSE_STOP);
   //add by allen 
	//return CAR_REVERSE_START;

#else
	int value = 1;
	car_reverse_set_int_ioin();
	value = gpio_get_value(car_reverse->reverse_gpio);
	printk("in allen car_reverse_gpio_status reverse_gpio = %d value = %d\n",car_reverse->reverse_gpio,value);
	car_reverse_set_io_int();
	return (value == 0) ? CAR_REVERSE_START : CAR_REVERSE_STOP;
#endif
}

/*
 *  current status | gpio status | next status
 *  ---------------+-------------+------------
 *        STOP     |    STOP     |    HOLD
 *  ---------------+-------------+------------
 *        STOP     |    START    |    START
 *  ---------------+-------------+------------
 *        START    |    STOP     |    STOP
 *  ---------------+-------------+------------
 *        START    |    START    |    HOLD
 *  ---------------+-------------+------------
 */
const int _transfer_table[3][3] = {
	[0] = {0, 0, 0},
	[CAR_REVERSE_START] = {0, CAR_REVERSE_HOLD, CAR_REVERSE_STOP},
	[CAR_REVERSE_STOP] = {0, CAR_REVERSE_START, CAR_REVERSE_HOLD},
};

static int car_reverse_get_next_status(void)
{
	int next_status;
	int gpio_status = car_reverse_gpio_status();
	int curr_status = car_reverse->status;
	//add by allen
	//gpio_status = CAR_REVERSE_START;
	
	car_reverse_switch_update(gpio_status == CAR_REVERSE_START ? 1 : 0);
	next_status = _transfer_table[curr_status][gpio_status];
    printk("gpio_status = %d\r\n",gpio_status);
	printk("curr_status = %d\r\n",curr_status);
	printk("next_status = %d\r\n",next_status);
	return next_status;
}

static void status_detect_func(struct work_struct *work)
{
	int retval;
	int status = car_reverse_get_next_status();
	if (car_reverse->standby)
		status = CAR_REVERSE_STOP;
	//printk("status_detect_func\n");

	switch (status) {
	case CAR_REVERSE_START:
		if (!car_reverse->needexit) {
            printk("CAR_REVERSE_START\r\n");
			retval = car_reverse_preview_start();
			logdebug("start car reverse, return %d\n", retval);
		}
		break;
	case CAR_REVERSE_STOP:
		retval = car_reverse_preview_stop();
		logdebug("stop car reverse, return %d\n", retval);
	    
		break;
	case CAR_REVERSE_HOLD:
	default:
	    printk("CAR_REVERSE_HOLD\r\n");
		break;
	}
	#ifdef USE_SERIAL
	car_status_flag = 0;
	#endif
	return;
}

static irqreturn_t reverse_irq_handle(int irqnum, void *data)
{
	printk(KERN_ERR "reverse_irq_handle\n");
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	return IRQ_HANDLED;
}

static ssize_t car_reverse_status_show(struct class *class,
				       struct class_attribute *attr, char *buf)
{
	int count = 0;

	if (car_reverse->status == CAR_REVERSE_STOP)
		count += sprintf(buf, "%s\n", "stop");
	else if (car_reverse->status == CAR_REVERSE_START)
		count += sprintf(buf, "%s\n", "start");
	else
		count += sprintf(buf, "%s\n", "unknow");
	return count;
}

static ssize_t car_reverse_format_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1))
		car_reverse->format = 1;
	else
		car_reverse->format = 0;

	return count;
}

static ssize_t car_reverse_format_show(struct class *class,
				       struct class_attribute *attr, char *buf)
{
	int count = 0;

	count += sprintf(buf, "%d\n", car_reverse->format);
	return count;
}

static ssize_t car_reverse_oview_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1)) {
		car_reverse->used_oview = 1;
	} else {
		car_reverse->used_oview = 0;
	}

	return count;
}

static ssize_t car_reverse_oview_show(struct class *class,
				      struct class_attribute *attr, char *buf)
{
	int count = 0;

	count += sprintf(buf, "%d\n", car_reverse->used_oview);
	return count;
}

static ssize_t car_reverse_src_store(struct class *class,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1)) {
		car_reverse->config.input_src = 1;
	} else {
		car_reverse->config.input_src = 0;
	}

	return count;
}

static ssize_t car_reverse_src_show(struct class *class,
				    struct class_attribute *attr, char *buf)
{
	int count = 0;

	count += sprintf(buf, "%d\n", car_reverse->config.input_src);
	return count;
}

static ssize_t car_reverse_rotation_store(struct class *class,
					  struct class_attribute *attr,
					  const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.rotation = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_rotation_show(struct class *class,
					 struct class_attribute *attr,
					 char *buf)
{
	int count = 0;

	count += sprintf(buf, "%d\n", car_reverse->config.rotation);
	return count;
}

static ssize_t car_reverse_needexit_store(struct class *class,
					  struct class_attribute *attr,
					  const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1))
		car_reverse->needexit = 1;
	else
		car_reverse->needexit = 0;
    //dump_stack();
	//close_special_serial();
	dump_stack();
	
	return count;
}

static ssize_t car_reverse_needexit_show(struct class *class,
					 struct class_attribute *attr,
					 char *buf)
{
	int count = 0;
	if (car_reverse->needexit == 1)
		count += sprintf(buf, "needexit = %d\n", 1);
	else
		count += sprintf(buf, "needexit = %d\n", 0);

	return count;
}
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
static ssize_t car_reverse_orientation_store(struct class *class,
					     struct class_attribute *attr,
					     const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.car_direct = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_orientation_show(struct class *class,
					    struct class_attribute *attr,
					    char *buf)
{
	int count = 0;

	count += sprintf(buf, "%d\n", car_reverse->config.car_direct);
	return count;
}

static ssize_t car_reverse_lrdirect_store(struct class *class,
					  struct class_attribute *attr,
					  const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.lr_direct = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_lrdirect_show(struct class *class,
					 struct class_attribute *attr,
					 char *buf)
{
	int count = 0;

	count += sprintf(buf, "%d\n", car_reverse->config.lr_direct);
	return count;
}

static ssize_t car_reverse_mirror_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.pr_mirror = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_mirror_show(struct class *class,
				       struct class_attribute *attr, char *buf)
{
	int count = 0;

	count += sprintf(buf, "%d\n", car_reverse->config.pr_mirror);
	return count;
}

#endif
#ifdef _REVERSE_DEBUG_
static ssize_t car_reverse_debug_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	if (!strncmp(buf, "stop", 4))
		car_reverse->debug = CAR_REVERSE_STOP;
	else if (!strncmp(buf, "start", 5))
		car_reverse->debug = CAR_REVERSE_START;

	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);

	return count;
}
#endif

static struct class_attribute car_reverse_attrs[] = {
    __ATTR(status, 0775, car_reverse_status_show, NULL),
    __ATTR(needexit, 0775, car_reverse_needexit_show,
	   car_reverse_needexit_store),
    __ATTR(format, 0775, car_reverse_format_show,
	   car_reverse_format_store),
    __ATTR(rotation, 0775, car_reverse_rotation_show,
	   car_reverse_rotation_store),
    __ATTR(src, 0775, car_reverse_src_show, car_reverse_src_store),
    __ATTR(oview, 0775, car_reverse_oview_show, car_reverse_oview_store),
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
    __ATTR(car_mirror, 0775, car_reverse_mirror_show,
	   car_reverse_mirror_store),
    __ATTR(car_direct, 0775, car_reverse_orientation_show,
	   car_reverse_orientation_store),
    __ATTR(car_lr, 0775, car_reverse_lrdirect_show,
	   car_reverse_lrdirect_store),
#endif
#ifdef _REVERSE_DEBUG_
    __ATTR(debug, 0775, NULL, car_reverse_debug_store),
#endif
    __ATTR_NULL};

static struct class car_reverse_class = {
    .name = "car_reverse", .class_attrs = car_reverse_attrs,
};


static int mcu_data_open(struct inode *inode, struct file *filp)
{
    if (mcu_data_prv == NULL)
        return -ENODEV;

	filp->private_data = mcu_data_prv;

	return 0;
}

static int mcu_data_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;

	return 0;
}

static long mcu_data_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	void __user *argp = (void __user *)arg;
	int value0;
	int value1 ;
	
	/* Verify user arguments. */
	//if (_IOC_TYPE(cmd) != REVERSE_IOC_MCU_DATA)
		//return -ENOTTY;
    printk("mcu_data_ioctl Enter\n");
	switch(cmd) {
	printk("mcu_data_ioctl Enter\n");
	case REVERSE_IOC_TEST_DATA:
		printk("mcu_data_TEST\n");
		if (copy_from_user(&value0, argp, sizeof(int))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		
	    printk("value0 = %d\r\n",value0);
		break;
	case REVERSE_IOC_MCU_DATA:
		printk("mcu_data_WRITE\n");
		if (copy_from_user(mcu_data_prv, argp, sizeof(struct mcu_data_t))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		printk("this is user(mcu) data:  11111\r\n");
		printk("smd_info=%d\n",mcu_data_prv->smd_info);
		printk("actual_gear=%d\n",mcu_data_prv->actual_gear);
		printk("steering_angle=%d\n",mcu_data_prv->steering_angle);
		printk("r_radar_distance=%d\n",mcu_data_prv->r_radar_distance);
		printk("m_radar_distance=%d\n",mcu_data_prv->m_radar_distance);
		printk("l_radar_distance=%d\n",mcu_data_prv->l_radar_distance);
		data_buf[0] = mcu_data_prv->smd_info;
		data_buf[1] = mcu_data_prv->actual_gear;
		data_buf[2] = mcu_data_prv->steering_angle;
		data_buf[3] = mcu_data_prv->r_radar_distance;
		data_buf[4] = mcu_data_prv->m_radar_distance;
		data_buf[5] = mcu_data_prv->l_radar_distance;
        ioctl_flag = 1;
        //up(&mcu_wakesem);
		break;
#if 1
	case REVERSE_IOC_RMCU_DATA:
		printk("mcu_data_READ\n");
		mcu_data_prv->smd_info=2;
		mcu_data_prv->actual_gear=2;
		mcu_data_prv->steering_angle=2;
		mcu_data_prv->r_radar_distance=2;
		mcu_data_prv->m_radar_distance=2;
		mcu_data_prv->l_radar_distance=2;
		if (copy_to_user(argp, mcu_data_prv, sizeof(struct mcu_data_t))) {
			printk("copy_to_user error\r\n");
			return -EFAULT;
		}

		break;
#endif
	default:
		printk("Invalid ioctl command.\n");
		return -ENOTTY;
	}

	return err;
}


static long mcu_data_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	void __user *argp = (void __user *)arg;
	int value0;
	int value1 ;
	
	/* Verify user arguments. */
	//if (_IOC_TYPE(cmd) != REVERSE_IOC_MCU_DATA)
		//return -ENOTTY;

    printk("mcu_data_compat_ioctl Enter\n");
	switch(cmd) {
	printk("mcu_data_compat_ioctl Enter\n");
	case REVERSE_IOC_TEST_DATA:
		printk("mcu_data_TEST\n");
		if (copy_from_user(&value0, argp, sizeof(int))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		
	    printk("value0 = %d\r\n",value0);
		break;
	case REVERSE_IOC_MCU_DATA:
		printk("mcu_data_WRITE\n");
		if (copy_from_user(mcu_data_prv, argp, sizeof(struct mcu_data_t))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		printk("this is user(mcu) data:  11111\r\n");
		printk("smd_info=%d\n",mcu_data_prv->smd_info);
		printk("actual_gear=%d\n",mcu_data_prv->actual_gear);
		printk("steering_angle=%d\n",mcu_data_prv->steering_angle);
		printk("r_radar_distance=%d\n",mcu_data_prv->r_radar_distance);
		printk("m_radar_distance=%d\n",mcu_data_prv->m_radar_distance);
		printk("l_radar_distance=%d\n",mcu_data_prv->l_radar_distance);
		data_buf[0] = mcu_data_prv->smd_info;
		data_buf[1] = mcu_data_prv->actual_gear;
		data_buf[2] = mcu_data_prv->steering_angle;
		data_buf[3] = mcu_data_prv->r_radar_distance;
		data_buf[4] = mcu_data_prv->m_radar_distance;
		data_buf[5] = mcu_data_prv->l_radar_distance;
		
        //up(&mcu_wakesem);
		break;
#if 1
	case REVERSE_IOC_RMCU_DATA:
		printk("mcu_data_READ\n");
		mcu_data_prv->smd_info=2;
		mcu_data_prv->actual_gear=2;
		mcu_data_prv->steering_angle=2;
		mcu_data_prv->r_radar_distance=2;
		mcu_data_prv->m_radar_distance=2;
		mcu_data_prv->l_radar_distance=2;
		if (copy_to_user(argp, mcu_data_prv, sizeof(struct mcu_data_t))) {
			printk("copy_to_user error\r\n");
			return -EFAULT;
		}
		
		break;
#endif
	default:
		printk("Invalid ioctl command.\n");
		return -ENOTTY;
	}

	return err;
}


static const struct file_operations mcu_data_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mcu_data_ioctl,
	.compat_ioctl = mcu_data_compat_ioctl,
	.open = mcu_data_open,
	.release = mcu_data_release
};

static struct miscdevice mcu_data_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEV_NAME,
	.fops = &mcu_data_fops
};

static int car_reverse_probe(struct platform_device *pdev)
{
	int retval = 0;
	int reverse_pin_irqnum;
#ifdef USE_SUNXI_DI_MODULE
	di_id = -1;
#endif

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "of_node is missing\n");
		retval = -EINVAL;
		goto _err_out;
	}

	car_reverse = devm_kzalloc(
	    &pdev->dev, sizeof(struct car_reverse_private_data), GFP_KERNEL);
	if (!car_reverse) {
		dev_err(&pdev->dev, "kzalloc for private data failed\n");
		retval = -ENOMEM;
		goto _err_out;
	}

	printk("mcu_data_probe Enter\r\n");
	mcu_data_prv = kzalloc(sizeof(struct mcu_data_t), GFP_KERNEL);

	if (mcu_data_prv == NULL) {
		printk("Not enough memory to initialize device\n");
		//return -ENOMEM;
    }

	retval = misc_register(&mcu_data_dev);
	if (retval < 0) {
		printk("mcu_data misc_register failed\r\n");
		kfree(mcu_data_prv);
		mcu_data_prv = NULL;
	}

	//sema_init(&mcu_wakesem,0);
    printk("mcu_data_probe Exit\r\n");

	platform_set_drvdata(pdev, car_reverse);
	parse_config(pdev, car_reverse);
	INIT_LIST_HEAD(&car_reverse->pending_frame);
	spin_lock_init(&car_reverse->display_lock);
	spin_lock_init(&car_reverse->thread_lock);
	spin_lock_init(&car_reverse->free_lock);
	car_reverse->needexit = 0;
	car_reverse->status = CAR_REVERSE_STOP;
	car_reverse->config.dev = &pdev->dev;
	sunxi_gpio_to_name(car_reverse->reverse_gpio, car_irq_pin_name);
	reverse_pin_irqnum = gpio_to_irq(car_reverse->reverse_gpio);
	if (IS_ERR_VALUE(reverse_pin_irqnum)) {
		dev_err(&pdev->dev,
			"map gpio [%d] to virq failed, errno = %d\n",
			car_reverse->reverse_gpio, reverse_pin_irqnum);
		retval = -EINVAL;
		goto _err_out;
	}

	car_reverse->preview_workqueue =
	    create_singlethread_workqueue("car-reverse-wq");
	if (!car_reverse->preview_workqueue) {
		dev_err(&pdev->dev, "create workqueue failed\n");
		retval = -EINVAL;
		goto _err_out;
	}
	INIT_WORK(&car_reverse->status_detect, status_detect_func);

	class_register(&car_reverse_class);
	car_reverse_switch_register();
	car_reverse->format = 0;
	car_reverse->standby = 0;
	car_reverse->config.car_oview_mode = car_reverse->used_oview;
	car_reverse->config.format = V4L2_PIX_FMT_NV21;
#ifdef USE_YUV422
	car_reverse->config.format = V4L2_PIX_FMT_NV61;
	car_reverse->format = 1;
#endif
	car_reverse->needfree = 0;

	/*
		if (car_reverse->config.input_src) {
			car_reverse->buffer_pool =
			    alloc_buffer_pool(car_reverse->config.dev,
					      SWAP_BUFFER_CNT_VIN, 1280 * 720 *
	   2);
		} else {
			car_reverse->buffer_pool = alloc_buffer_pool(
			    car_reverse->config.dev, SWAP_BUFFER_CNT, 720 * 576
	   * 2);
			car_reverse->buffer_disp[0] =
			    __buffer_node_alloc(car_reverse->config.dev, 720 *
	   576 * 2);
			car_reverse->buffer_disp[1] =
			    __buffer_node_alloc(car_reverse->config.dev, 720 *
	   576 * 2);
		}
	*/
	
#ifdef USE_SERIAL	
    printk("serial_parse_work start\r\n");

	if (car_reverse->reverse_can_used) {
		car_reverse->serial_task =
		    kthread_create(serial_parse_work, NULL, "car_serial");
		if (!car_reverse->serial_task) {
			printk(KERN_ERR "failed to create serial_task kthread\n");
		}
		else {
			printk("successed to create serial_task kthread\n");
		}
	}
	if (car_reverse->serial_task) {
		printk("serial_task wake_up_process\r\n");
		wake_up_process(car_reverse->serial_task);
	}
#else
    printk("mcu_data_work start\r\n");

	
	car_reverse->mcu_data_task = kthread_create(mcu_data_work, NULL, "mcu_data");
	if (!car_reverse->mcu_data_task) {
		printk(KERN_ERR "failed to create mcu_data_task kthread\n");
	}
	else {
		printk("successed to create mcu_data_task kthread\n");
	}
	
	if (car_reverse->mcu_data_task) {
		printk("mcu_data_task wake_up_process\r\n");
		wake_up_process(car_reverse->mcu_data_task);
	}
#endif	
	printk(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);

	if (request_irq(reverse_pin_irqnum, reverse_irq_handle,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"car-reverse", pdev)) {

		dev_err(&pdev->dev, "request irq %d failed\n",
			reverse_pin_irqnum);
		retval = -EBUSY;
		goto _err_free_buffer;
	}
			
	car_reverse->debug = CAR_REVERSE_STOP;
	//car_reverse->debug = CAR_REVERSE_START;
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);

	dev_info(&pdev->dev, "car reverse module probe ok\n");
	return 0;

_err_free_buffer:
/*
	free_buffer_pool(car_reverse->config.dev, car_reverse->buffer_pool);
	if (car_reverse->config.input_src == 0) {
		__buffer_node_free(car_reverse->config.dev,
				   car_reverse->buffer_disp[0]);
		__buffer_node_free(car_reverse->config.dev,
				   car_reverse->buffer_disp[1]);
	}
*/
_err_out:
	dev_err(&pdev->dev, "car reverse module exit, errno %d!\n", retval);
	return retval;
}

static int car_reverse_remove(struct platform_device *pdev)
{
	struct car_reverse_private_data *priv = car_reverse;
	/*
  free_buffer_pool(car_reverse->config.dev, car_reverse->buffer_pool);
	if (car_reverse->config.input_src == 0) {
		__buffer_node_free(car_reverse->config.dev,
				   car_reverse->buffer_disp[0]);
		__buffer_node_free(car_reverse->config.dev,
				   car_reverse->buffer_disp[1]);
	}
	*/
	kfree(mcu_data_prv);
	mcu_data_prv = NULL;
	misc_deregister(&mcu_data_dev);
	printk("mcu_data remove\n");

	printk("mcu_data_task kthread_stop\r\n");
	if (car_reverse->mcu_data_task)
		kthread_stop(car_reverse->mcu_data_task);
	car_reverse->mcu_data_task = 0;
	
	car_reverse_switch_unregister();
	class_unregister(&car_reverse_class);
	free_irq(gpio_to_irq(priv->reverse_gpio), pdev);

	cancel_work_sync(&priv->status_detect);
	if (priv->preview_workqueue != NULL) {
		flush_workqueue(priv->preview_workqueue);
		destroy_workqueue(priv->preview_workqueue);
		priv->preview_workqueue = NULL;
	}

	dev_info(&pdev->dev, "car reverse module exit\n");
	return 0;
}

static int car_reverse_suspend(struct device *dev)
{
	if (car_reverse->status == CAR_REVERSE_START) {
		car_reverse->standby = 1;
		flush_workqueue(car_reverse->preview_workqueue);
	}
	return 0;
}

static int car_reverse_resume(struct device *dev)
{

	if (car_reverse->standby) {
		car_reverse->standby = 0;
		queue_work(car_reverse->preview_workqueue,
			   &car_reverse->status_detect);
	}
	return 0;
}

static const struct dev_pm_ops car_reverse_pm_ops = {
    .suspend = car_reverse_suspend, .resume = car_reverse_resume,
};

static const struct of_device_id car_reverse_dt_ids[] = {
    {.compatible = "allwinner,sunxi-car-reverse"}, {},
};

static struct platform_driver car_reverse_driver = {
    .probe = car_reverse_probe,
    .remove = car_reverse_remove,
    .driver = {
	    .name = MODULE_NAME,
	    .pm = &car_reverse_pm_ops,
	    .of_match_table = car_reverse_dt_ids,
	},
};

static int __init car_reverse_module_init(void)
{
	int ret;

	ret = platform_driver_register(&car_reverse_driver);
	if (ret) {
		pr_err("platform driver register failed\n");
		return ret;
	}

	return 0;
}

static void __exit car_reverse_module_exit(void)
{
	platform_driver_unregister(&car_reverse_driver);
}

subsys_initcall_sync(car_reverse_module_init);
//module_init(car_reverse_module_init);
module_exit(car_reverse_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zeng.Yajian <zengyajian@allwinnertech.com>");
MODULE_DESCRIPTION("Sunxi fast car reverse image preview");
