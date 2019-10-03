/*
 * Fast car reverse, auxiliary line
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

#include "include.h"
#include <linux/delay.h>
#include "colormap.h"
#include "upng.h"
#include "ubmp.h"
#include <asm/cacheflush.h>
#include <asm/glue-cache.h>
#include "evs_communication.h"


extern bmp_img_t bmp_img;
extern int clear_cache(int screen_id, void *data,int data_w, int data_h, int p_byte);

extern int bmp_read_from_file(const char *name, void *base);
extern int analysis_of_radar_pictures(const char *bmp_path, const char *radar_path, void *base);

extern upng_t* bmp_new_from_file(const char *name);
//extern int bmp_read_from_file(const char *name);

extern int dec_png_fb(const char *path, void *buf, const int width, const int height);
extern int draw_image(void *base, unsigned int x, unsigned int y, unsigned int w,
	       unsigned int h, int stride_w, int stride_h, unsigned char *data,
	       unsigned int len);

void bmp_test(void *base);
void select_picture(void *base);


enum picture_info_type {
   ACTUAL_GEAR_PIC = 0,
   LINE_PIC = 1,
   R_RADAR_PIC = 2,
   M_RADAR_PIC = 3,
   L_RADAR_PIC = 4,    
};


struct argb_t {
	unsigned char transp;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

typedef void (*colormap_func)(int x, int y, int rotate, int lr,
			      struct argb_t *out, int w, int h, int dx);

const static int sin_table[] = {
    0,  1,  3,  5,  6,  8,  10, 12, 13, 15, 17, 19, 20, 22, 24, 25, 27, 29, 30,
    32, 34, 35, 37, 39, 40, 42, 43, 45, 46, 48, 50, 51, 52, 54, 55, 57, 58, 60,
    61, 62, 64, 65, 66, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
    83, 84, 85, 86, 87, 88, 89, 89, 90, 91, 92, 92, 93, 93, 94, 95, 95, 96, 96,
    97, 97, 97, 98, 98, 98, 99, 99, 99, 99, 99, 99, 99, 99, 100};

const static int cos_table[] = {
    100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 97, 97, 97, 96, 96, 95, 95,
    94,  93, 93, 92, 92, 91, 90, 89, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79,
    78,  77, 76, 75, 74, 73, 71, 70, 69, 68, 66, 65, 64, 62, 61, 60, 58, 57, 55,
    54,  52, 51, 50, 48, 46, 45, 43, 42, 40, 39, 37, 35, 34, 32, 30, 29, 27, 25,
    24,  22, 20, 19, 17, 15, 13, 12, 10, 8,  6,  5,  3,  1,  0};

struct canvas {
	char *base;
	int width;
	int height;
	int bpp;
	int stride;
};

static int LZou;
static int xgap;
static int ygap;
static int SensorAgle;
static int Car_Orientation;
static int Car_Lr;
static int Max_Agle;
#define Abs(x) (x >= 0 ? x : (-x))

static int Cal_CarTrail(int x, int y, int rotate)
{
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	int i = 0;
	int dx = 0, dy = 0;
	if (y < ygap) {
		return 0xFFFF;
	}
	if (rotate == 0) {
		return 0;
	}
	a1 = LZou * (cos_table[rotate]) / sin_table[rotate];
	a2 = a1;
	while (1) {
		int tmp = 0;
		dy = y - ygap;
		tmp = (dy * dy + i * i) - a1 * a1;
		if (Abs(a2) >= Abs(tmp)) {
			a2 = tmp;
			dx = i;
		}
		i++;
		if (i >= a1)
			break;
	}
	if (i == a1 && dx == 0)
		return 0xFFFF;
	a3 = a1 - dx;
	return a3;
}

static int draw_by_func(struct canvas *ca, int rotate, int lr,
			colormap_func func)
{
	int i, j;
	struct argb_t color;
	char *lines_start;
	unsigned char *from;
	int dx = 0;
	lines_start = (char *)ca->base;
	for (i = 0; i < ca->height; i++) {
		from = lines_start;
		dx = Cal_CarTrail(j, i, rotate);
		for (j = 0; j < ca->width; j++) {
			/*func(j, i, &color);*/
			func(j, i, rotate, lr, &color, ca->width, ca->height,
			     dx);
			*from++ = color.blue;
			*from++ = color.green;
			*from++ = color.red;
			*from++ = color.transp;
		}
		lines_start += ca->stride;
	}
	return 0;
}

static void colormap(int x, int y, int rotate, int lr, struct argb_t *out,
		     int w, int h, int dx)

{
	struct argb_t unvisable = {0x00, 0x00, 0x00, 0x00};
	struct argb_t red = {0xff, 0xff, 0x00, 0x00};
	struct argb_t yellow = {0xff, 0xff, 0xFF, 0x00};
	struct argb_t green = {0xff, 0x00, 0xf7, 0x00};
	struct argb_t *color;
	/*auxiliary line*/
	int dSensor = 0;
	int seg = (h - ygap) / 4;
	if (y < ygap) {
		out->red = unvisable.red;
		out->green = unvisable.green;
		out->blue = unvisable.blue;
		out->transp = unvisable.transp;
		return;
	}
	dSensor = (y - ygap) * sin_table[SensorAgle] / cos_table[SensorAgle];
	if ((y - ygap) == 0 || (y - ygap) == 1 || (y - ygap) == 2 ||
	    (y - ygap) == seg || ((y - ygap) == seg + 1) ||
	    ((y - ygap) == seg + 2) || (y - ygap) == (2 * seg) ||
	    (y - ygap) == (2 * seg + 1) || (y - ygap) == (2 * seg + 2)) {
		if ((((w - xgap) / 2 - dSensor) <= x) &&
		    ((w + xgap) / 2 + dSensor) >= x) {
			out->transp = green.transp;
			out->red = green.red;
			out->green = green.green;
			out->blue = green.blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	} else if ((y - ygap) == (3 * seg) || (y - ygap) == (3 * seg + 1) ||
		   (y - ygap) == (3 * seg + 2)) {
		if ((((w - xgap) / 2 - dSensor) <= x) &&
		    ((w + xgap) / 2 + dSensor) >= x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	} else if ((y - ygap) == (4 * seg) || (y - ygap) == (4 * seg - 1) ||
		   (y - ygap) == (4 * seg - 2)) {
		if ((((w - xgap) / 2 - dSensor) <= x) &&
		    ((w + xgap) / 2 + dSensor) >= x) {
			out->transp = red.transp;
			out->red = red.red;
			out->green = red.green;
			out->blue = red.blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	} else {
		if ((y - ygap) >= 0 && (y - ygap) <= (2 * seg + 2)) {
			color = &green;
		} else if ((y - ygap) >= (2 * seg + 2) &&
			   (y - ygap) <= (3 * seg + 2)) {
			color = &yellow;
		} else {
			color = &red;
		}
		if ((w - xgap) / 2 - dSensor == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w - xgap) / 2 - 1 - dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w - xgap) / 2 - 2 - dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w - xgap) / 2 - 3 - dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if ((w + xgap) / 2 + dSensor == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w + xgap) / 2 + 1 + dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w + xgap) / 2 + 2 + dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w + xgap) / 2 + 3 + dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	}
	if (dx > ((w - xgap) / 2) || dx < 0) {
		return;
	}
	if (rotate <= Max_Agle && (lr == 0)) {
		if ((w - xgap) / 2 - dx - 4 - dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 - dx - 5 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 - dx - 6 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 - dx - 7 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if ((w + xgap) / 2 - dx + 4 + dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 - dx + 5 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 - dx + 6 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 - dx + 7 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		}
	}
	if (rotate <= Max_Agle && lr) {
		if ((w - xgap) / 2 + dx - 4 - dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 + dx - 5 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 + dx - 6 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 + dx - 7 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if ((w + xgap) / 2 + dx + 4 + dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 + dx + 5 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 + dx + 6 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 + dx + 7 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		}
	}
	return;
}

void init_auxiliary_paramter(int ld, int hgap, int vgap, int sA, int mA)
{
	LZou = ld;
	xgap = hgap;
	ygap = vgap;
	SensorAgle = sA;
	Car_Orientation = -1;
	Max_Agle = mA;
	Car_Lr = -1;
	return;
}
#define __PNG_DECODER_TEST__


int draw_auxiliary_line(void *base, int width, int height, int rotate, int lr)
{
		struct canvas canvas;        
        unsigned char picdata[sizeof(color_map)] = {0};

		//if (Car_Orientation == rotate && lr == Car_Lr)
			//return 0;
		Car_Orientation = rotate;
		Car_Lr = lr;

		int ret = 0;
		
		canvas.base = base;
		canvas.width = width;
		canvas.height = height;
		canvas.bpp = 32;
		canvas.stride = width * 4;
		
		//draw_by_func(&canvas, rotate, lr, colormap);

		//bmp_test(base);
		select_picture(base);
		return 0;
}

int draw_upng_fb(void *base, int width, int height, int rotate, int lr)
{
	int ret = 0;
	struct file *filp;
    static int rootfs_mounted = 0;
#undef __PNG_DECODER_TEST__
#define __PNG_DECODER_TEST__
#ifdef __PNG_DECODER_TEST__
	char path[32] = {0};
	static int png_idx = 1;
#endif
	extern int dec_png_fb(const char *path, void *buf, const int width, const int height);
	if(rootfs_mounted == 0){
	   filp = filp_open("/system/usr/upng.png", O_RDONLY, 0);
	   if(IS_ERR(filp))
		   return 0;
	   filp_close(filp, NULL);
	   rootfs_mounted = 1;
	}
#ifdef __PNG_DECODER_TEST__
	if(png_idx > 1)
		png_idx = 1;
	snprintf(path, sizeof(path), "/system/usr/upng.png", png_idx);
	//snprintf(path, sizeof(path), "/usr/back_r_%d.png", png_idx);
	png_idx ++;
#endif
#undef __PNG_DECODER_TEST__
	ret = dec_png_fb(path, base, (const int)width, (const int)height);
	if(ret < 0)
		printk(KERN_WARNING "decoder failed \n");

	return ret;
}


int draw_image(void *base, unsigned int x, unsigned int y, unsigned int w,
	       unsigned int h, int stride_w, int stride_h, unsigned char *data,
	       unsigned int len)
{
	int i, j;
	struct argb_t color;
	char *lines_start;
	unsigned char *from;
	unsigned char *p = data;
	unsigned int count = 0;
	struct canvas canvas;
	canvas.base = base;
	canvas.width = stride_w;
	canvas.height = stride_h;
	canvas.bpp = 32;
	canvas.stride = stride_w * 4;
	lines_start = (char *)(canvas.base + x + y * canvas.stride);
	from = lines_start;

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			if (count <= len) {
				color.blue = *(p + 2);
				color.green = *(p);
				color.red = *(p + 1);
				color.transp = 0xff;
			} else {
				color.blue = 0;
				color.green = 0;
				color.red = 0;
				color.transp = 0xff;
			}
			*from++ = color.blue;
			*from++ = color.green;
			*from++ = color.red;
			*from++ = color.transp;
			count += 3;
			p += 3;
		}
		lines_start += canvas.stride;
		from = lines_start;
	}
	return 0;
}

extern int car_status_flag;
extern struct parse_mcu_data_s *parse_mcu_data;
extern int parse_data_idx[5];
extern int parse_data[5];
//extern unsigned char parse_data_type[5];

#if 0
int print_parse_mcu_data(int num)
{
    int i = 0;

	for(i = 0; i < num; i++) {
        printk("parse_mcu_data[%d].data = %d\r\n",i,parse_mcu_data[i].data);
		printk("parse_mcu_data[%d].idx = %d\r\n",i,parse_mcu_data[i].idx);
	    printk("parse_mcu_data[%d].type = %s\r\n",i,parse_mcu_data[i].type);
		printk("\r\n");
	}
    
    return 0;
}
#else
int print_parse_mcu_data(int num)
{
    int i = 0;

	for(i = 0; i < num; i++) {
        printk(" %d %d\r\n",i,parse_data[i], parse_data_idx[i]);
	}
    
    return 0;
}

#endif

void select_picture(void *base)
{
    //printk("car_status_flag = %d\r\n",car_status_flag);

    if(car_status_flag) {
		//print_parse_mcu_data(5);
    snprintf(bmp_img.carmodel_path, sizeof(bmp_img.carmodel_path), "/system/usr/carmodel.bmp");    
	snprintf(bmp_img.line_path, sizeof(bmp_img.line_path), "/system/usr/line_%d.bmp", parse_data_idx[LINE_PIC]);
	snprintf(bmp_img.radar_r_path, sizeof(bmp_img.radar_r_path), "/system/usr/radar_r_%d.bmp", parse_data_idx[R_RADAR_PIC]);
	snprintf(bmp_img.radar_m_path, sizeof(bmp_img.radar_m_path), "/system/usr/radar_m_%d.bmp", parse_data_idx[M_RADAR_PIC]);
	snprintf(bmp_img.radar_l_path, sizeof(bmp_img.radar_l_path), "/system/usr/radar_l_%d.bmp", parse_data_idx[L_RADAR_PIC]);

	//printk("path = %s\r\n",bmp_img.line_path);
	//printk("path = %s\r\n",bmp_img.radar_r_path);
	//printk("path = %s\r\n",bmp_img.radar_m_path);
	//printk("path = %s\r\n",bmp_img.radar_l_path);
    memset(base, 0 ,1920*720*4);
	dmac_flush_range(base, base+1920*720*4);
	
	analysis_carmodel_pictures(bmp_img.carmodel_path, base);
	car_model_rgb_to_argb(bmp_img.carmodel_dest, bmp_img.carmodel_src, bmp_img.carmodel_w, bmp_img.carmodel_h);//将图片转码
    copy_to_base(base, bmp_img.carmodel_dest, bmp_img.carmodel_w, bmp_img.carmodel_h, 0, 0);//图片数据复制到 base frame 显示

	analysis_auxiliary_line_pictures(bmp_img.line_path, base);
	rgb_to_argb(bmp_img.line_dest, bmp_img.line_src, bmp_img.line_w, bmp_img.line_h);//将图片转码
	copy_to_base(base, bmp_img.line_dest, bmp_img.line_w, bmp_img.line_h, 940, 240);//图片数据复制到 base frame 显示

	switch (parse_data_idx[R_RADAR_PIC]) {
		case 0:
			break;
		case 1:
			analysis_radar_r_g_pictures(bmp_img.radar_r_path, base);
			rgb_to_argb(bmp_img.radar_r_g_dest, bmp_img.radar_r_g_src, bmp_img.radar_r_g_w, bmp_img.radar_r_g_h);//将图片转码
			radar_copy_to_base(base, bmp_img.radar_r_g_dest, bmp_img.radar_r_g_w, bmp_img.radar_r_g_h, 661, 566);//图片数据复制到 base frame 显示
			break;
		case 2:
			analysis_radar_r_o_pictures(bmp_img.radar_r_path, base);
			rgb_to_argb(bmp_img.radar_r_o_dest, bmp_img.radar_r_o_src, bmp_img.radar_r_o_w, bmp_img.radar_r_o_h);//将图片转码
			radar_copy_to_base(base, bmp_img.radar_r_o_dest, bmp_img.radar_r_o_w, bmp_img.radar_r_o_h, 651, 548);//图片数据复制到 base frame 显示
			break;
		case 3:
			analysis_radar_r_r_pictures(bmp_img.radar_r_path, base);
			rgb_to_argb(bmp_img.radar_r_r_dest, bmp_img.radar_r_r_src, bmp_img.radar_r_r_w, bmp_img.radar_r_r_h);//将图片转码
			radar_copy_to_base(base, bmp_img.radar_r_r_dest, bmp_img.radar_r_r_w, bmp_img.radar_r_r_h, 640, 529);//图片数据复制到 base frame 显示
			break;
		default:
		    break;
	}


	switch (parse_data_idx[M_RADAR_PIC]) {
		case 0:
			break;
		case 1:
			analysis_radar_m_g_pictures(bmp_img.radar_m_path, base);
			rgb_to_argb(bmp_img.radar_m_g_dest, bmp_img.radar_m_g_src, bmp_img.radar_m_g_w, bmp_img.radar_m_g_h);//将图片转码
			radar_copy_to_base(base, bmp_img.radar_m_g_dest, bmp_img.radar_m_g_w, bmp_img.radar_m_g_h, 522, 566);//图片数据复制到 base frame 显示
			break;
		case 2:
			analysis_radar_m_o_pictures(bmp_img.radar_m_path, base);
			rgb_to_argb(bmp_img.radar_m_o_dest, bmp_img.radar_m_o_src, bmp_img.radar_m_o_w, bmp_img.radar_m_o_h);//将图片转码
			radar_copy_to_base(base, bmp_img.radar_m_o_dest, bmp_img.radar_m_o_w, bmp_img.radar_m_o_h, 532, 548);//图片数据复制到 base frame 显示
			break;
		case 3:
			analysis_radar_m_r_pictures(bmp_img.radar_m_path, base);
			rgb_to_argb(bmp_img.radar_m_r_dest, bmp_img.radar_m_r_src, bmp_img.radar_m_r_w, bmp_img.radar_m_r_h);//将图片转码
			radar_copy_to_base(base, bmp_img.radar_m_r_dest, bmp_img.radar_m_r_w, bmp_img.radar_m_r_h, 540, 529);//图片数据复制到 base frame 显示
			break;
		default:
		    break;
	}


	switch (parse_data_idx[L_RADAR_PIC]) {
		case 0:
			break;
		case 1:
			analysis_radar_l_g_pictures(bmp_img.radar_l_path, base);
			rgb_to_argb(bmp_img.radar_l_g_dest, bmp_img.radar_l_g_src, bmp_img.radar_l_g_w, bmp_img.radar_l_g_h);//将图片转码
            radar_copy_to_base(base, bmp_img.radar_l_g_dest, bmp_img.radar_l_g_w, bmp_img.radar_l_g_h, 457, 566);//图片数据复制到 base frame 显示
			break;
		case 2:
			analysis_radar_l_o_pictures(bmp_img.radar_l_path, base);
			rgb_to_argb(bmp_img.radar_l_o_dest, bmp_img.radar_l_o_src, bmp_img.radar_l_o_w, bmp_img.radar_l_o_h);//将图片转码
            radar_copy_to_base(base, bmp_img.radar_l_o_dest, bmp_img.radar_l_o_w, bmp_img.radar_l_o_h, 475, 548);//图片数据复制到 base frame 显示
			break;
		case 3:
			analysis_radar_l_r_pictures(bmp_img.radar_l_path, base);
            rgb_to_argb(bmp_img.radar_l_r_dest, bmp_img.radar_l_r_src, bmp_img.radar_l_r_w, bmp_img.radar_l_r_h);//将图片转码
            radar_copy_to_base(base, bmp_img.radar_l_r_dest, bmp_img.radar_l_r_w, bmp_img.radar_l_r_h, 493, 529);//图片数据复制到 base frame 显示
            break;
		default:
		    break;
	}
	
	dmac_flush_range(base, base+1920*720*4);
	car_status_flag = 0;
	printk("a frame finnished\r\n");
    }
	
}

#if 0
void bmp_test(void *base)
{
	static int bmp_idx = 0;
	
	if(bmp_idx > 5)
		bmp_idx = 0;
	bmp_idx ++;

    printk("car_status_flag = %d\r\n",car_status_flag);
    if(car_status_flag) {
		print_parse_mcu_data(5);
    }
	
	snprintf(bmp_img.carmodel_path, sizeof(bmp_img.carmodel_path), "/system/usr/carmodel.bmp");
	switch(bmp_idx) {
	case 1:
		bmp_img.line_idx = bmp_idx;
		bmp_img.radar_r_idx = bmp_idx; 
		bmp_img.radar_m_idx = bmp_idx;
		bmp_img.radar_l_idx = bmp_idx;
		snprintf(bmp_img.line_path, sizeof(bmp_img.line_path), "/system/usr/line_%d.bmp", bmp_img.line_idx);
		snprintf(bmp_img.radar_r_path, sizeof(bmp_img.radar_r_path), "/system/usr/radar_r_%d.bmp", bmp_img.radar_r_idx);
		snprintf(bmp_img.radar_m_path, sizeof(bmp_img.radar_m_path), "/system/usr/radar_m_%d.bmp", bmp_img.radar_m_idx);
		snprintf(bmp_img.radar_l_path, sizeof(bmp_img.radar_l_path), "/system/usr/radar_l_%d.bmp", bmp_img.radar_l_idx);
		break;
	case 2:
		bmp_img.line_idx = bmp_idx;
		bmp_img.radar_r_idx = bmp_idx; 
		bmp_img.radar_m_idx = bmp_idx;
		bmp_img.radar_l_idx = bmp_idx;
		snprintf(bmp_img.line_path, sizeof(bmp_img.line_path), "/system/usr/line_%d.bmp", bmp_img.line_idx);
		snprintf(bmp_img.radar_r_path, sizeof(bmp_img.radar_r_path), "/system/usr/radar_r_%d.bmp", bmp_img.radar_r_idx);
		snprintf(bmp_img.radar_m_path, sizeof(bmp_img.radar_m_path), "/system/usr/radar_m_%d.bmp", bmp_img.radar_m_idx);
		snprintf(bmp_img.radar_l_path, sizeof(bmp_img.radar_l_path), "/system/usr/radar_l_%d.bmp", bmp_img.radar_l_idx);
		break;
	case 3:
		bmp_img.line_idx = bmp_idx;
		bmp_img.radar_r_idx = bmp_idx; 
		bmp_img.radar_m_idx = bmp_idx;
		bmp_img.radar_l_idx = bmp_idx;
		snprintf(bmp_img.line_path, sizeof(bmp_img.line_path), "/system/usr/line_%d.bmp", bmp_img.line_idx);
		snprintf(bmp_img.radar_r_path, sizeof(bmp_img.radar_r_path), "/system/usr/radar_r_%d.bmp", bmp_img.radar_r_idx);
		snprintf(bmp_img.radar_m_path, sizeof(bmp_img.radar_m_path), "/system/usr/radar_m_%d.bmp", bmp_img.radar_m_idx);
		snprintf(bmp_img.radar_l_path, sizeof(bmp_img.radar_l_path), "/system/usr/radar_l_%d.bmp", bmp_img.radar_l_idx);
		break;
	case 4:
		bmp_img.line_idx = bmp_idx;
		bmp_img.radar_r_idx = bmp_idx; 
		bmp_img.radar_m_idx = bmp_idx;
		bmp_img.radar_l_idx = bmp_idx;
		snprintf(bmp_img.line_path, sizeof(bmp_img.line_path), "/system/usr/line_%d.bmp", bmp_img.line_idx);
		snprintf(bmp_img.radar_r_path, sizeof(bmp_img.radar_r_path), "/system/usr/radar_r_%d.bmp", bmp_img.radar_r_idx);
		snprintf(bmp_img.radar_m_path, sizeof(bmp_img.radar_m_path), "/system/usr/radar_m_%d.bmp", bmp_img.radar_m_idx);
		snprintf(bmp_img.radar_l_path, sizeof(bmp_img.radar_l_path), "/system/usr/radar_l_%d.bmp", bmp_img.radar_l_idx);
		break;
	case 5:
		bmp_img.line_idx = bmp_idx;
		bmp_img.radar_r_idx = bmp_idx; 
		bmp_img.radar_m_idx = bmp_idx;
		bmp_img.radar_l_idx = bmp_idx;
		snprintf(bmp_img.line_path, sizeof(bmp_img.line_path), "/system/usr/line_%d.bmp", bmp_img.line_idx);
		snprintf(bmp_img.radar_r_path, sizeof(bmp_img.radar_r_path), "/system/usr/radar_r_%d.bmp", bmp_img.radar_r_idx);
		snprintf(bmp_img.radar_m_path, sizeof(bmp_img.radar_m_path), "/system/usr/radar_m_%d.bmp", bmp_img.radar_m_idx);
		snprintf(bmp_img.radar_l_path, sizeof(bmp_img.radar_l_path), "/system/usr/radar_l_%d.bmp", bmp_img.radar_l_idx);
		break;
	default:
		break;
	}

	printk("path = %s\r\n",bmp_img.line_path);
	printk("path = %s\r\n",bmp_img.radar_r_path);
	printk("path = %s\r\n",bmp_img.radar_m_path);
	printk("path = %s\r\n",bmp_img.radar_l_path);
	
	msleep(5000);
	memset(base, 0 ,1920*720*4);
	dmac_flush_range(base, base+1920*720*4);
	analysis_auxiliary_line_pictures(bmp_img.line_path, base);
	analysis_carmodel_pictures(bmp_img.carmodel_path, base);
	analysis_radar_r_pictures(bmp_img.radar_r_path, base);
	analysis_radar_m_pictures(bmp_img.radar_m_path, base);
	analysis_radar_l_pictures(bmp_img.radar_l_path, base);
	
	rgb_to_argb(bmp_img.line_dest, bmp_img.line_src, bmp_img.line_w, bmp_img.line_h);//将图片转码
	rgb_to_argb(bmp_img.carmodel_dest, bmp_img.carmodel_src, bmp_img.carmodel_w, bmp_img.carmodel_h);//将图片转码
	rgb_to_argb(bmp_img.radar_r_dest, bmp_img.radar_r_src, bmp_img.radar_r_w, bmp_img.radar_r_h);//将图片转码
	rgb_to_argb(bmp_img.radar_m_dest, bmp_img.radar_m_src, bmp_img.radar_m_w, bmp_img.radar_m_h);//将图片转码
	rgb_to_argb(bmp_img.radar_l_dest, bmp_img.radar_l_src, bmp_img.radar_l_w, bmp_img.radar_l_h);//将图片转码 
	
	//clear_cache(base, 1920, 720, 4);
	copy_to_base(base, bmp_img.line_dest, bmp_img.line_w, bmp_img.line_h, 960, 240);//图片数据复制到 base frame 显示
	copy_to_base(base, bmp_img.carmodel_dest, bmp_img.carmodel_w, bmp_img.carmodel_h, 270, 160);//图片数据复制到 base frame 显示
	copy_to_base(base, bmp_img.radar_r_dest, bmp_img.radar_r_w, bmp_img.radar_r_h, 150, 40);//图片数据复制到 base frame 显示
	copy_to_base(base, bmp_img.radar_m_dest, bmp_img.radar_m_w, bmp_img.radar_m_h, 300, 560);//图片数据复制到 base frame 显示
	copy_to_base(base, bmp_img.radar_l_dest, bmp_img.radar_l_w, bmp_img.radar_l_h, 450, 40);//图片数据复制到 base frame 显示
    dmac_flush_range(base, base+1920*720*4);

	//car_status_flag = 0;
}
#endif
