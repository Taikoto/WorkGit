#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include "asm/cacheflush.h"
#include "ubmp.h"


bmp_img_t bmp_img;
dma_phy_t dma_phy;


int init_picture(void)
{
	bmp_img.argb_bit = 4;
	bmp_img.rgb_bit = 3;
	bmp_img.line_w = 880;
	bmp_img.line_h = 480;
	
	bmp_img.carmodel_w = 840;
	bmp_img.carmodel_h = 720;

	bmp_img.radar_r_g_w = 92;
	bmp_img.radar_r_g_h = 112;
	bmp_img.radar_r_o_w = 84;
	bmp_img.radar_r_o_h = 104;
	bmp_img.radar_r_r_w = 76;
	bmp_img.radar_r_r_h = 92;

	bmp_img.radar_m_g_w = 160;
	bmp_img.radar_m_g_h = 112;
	bmp_img.radar_m_o_w = 140;
	bmp_img.radar_m_o_h = 104;
	bmp_img.radar_m_r_w = 124;
	bmp_img.radar_m_r_h = 92;

	bmp_img.radar_l_g_w = 88;
	bmp_img.radar_l_g_h = 112;
	bmp_img.radar_l_o_w = 80;
	bmp_img.radar_l_o_h = 104;
	bmp_img.radar_l_r_w = 72;
	bmp_img.radar_l_r_h = 92;

	return 0;
}

#if 0
int alloc_memery(void)
{
	/*
	alloc rgb analysis_pictures memery
	alloc argb analysis_pictures memery
	alloc rgb radar_pictures memery
	alloc argb radar_pictures memery
	*/
	
	printk("%s Enter\r\n",__FUNCTION__);
	
	bmp_img.line_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.line_w*bmp_img.line_h, GFP_KERNEL);
	if (bmp_img.line_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.line_w,bmp_img.line_h);
		return -1;
	}
	
	bmp_img.line_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.line_w*bmp_img.line_h, GFP_KERNEL);
	if (bmp_img.line_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.line_w,bmp_img.line_h);
		return -1;
	}

	bmp_img.carmodel_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.carmodel_w*bmp_img.carmodel_h, GFP_KERNEL);
	if (bmp_img.carmodel_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.carmodel_w,bmp_img.carmodel_h);
		return -1;
	}
	
	bmp_img.carmodel_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.carmodel_w*bmp_img.carmodel_h, GFP_KERNEL);
	if (bmp_img.carmodel_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.carmodel_w,bmp_img.carmodel_h);
		return -1;
	}

	bmp_img.radar_r_g_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_r_g_w*bmp_img.radar_r_g_h, GFP_KERNEL);
	if (bmp_img.radar_r_g_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_r_g_w,bmp_img.radar_r_g_h);
		return -1;
	}
	bmp_img.radar_r_o_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_r_o_w*bmp_img.radar_r_o_h, GFP_KERNEL);
	if (bmp_img.radar_r_o_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_r_o_w,bmp_img.radar_r_o_h);
		return -1;
	}
	bmp_img.radar_r_r_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_r_r_w*bmp_img.radar_r_r_h, GFP_KERNEL);
	if (bmp_img.radar_r_r_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_r_r_w,bmp_img.radar_r_r_h);
		return -1;
	}

	bmp_img.radar_m_g_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_m_g_w*bmp_img.radar_m_g_h, GFP_KERNEL);
	if (bmp_img.radar_m_g_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_m_g_w,bmp_img.radar_m_g_h);
		return -1;
	}
	bmp_img.radar_m_o_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_m_o_w*bmp_img.radar_m_o_h, GFP_KERNEL);
	if (bmp_img.radar_m_o_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_m_o_w,bmp_img.radar_m_o_h);
		return -1;
	}
	bmp_img.radar_m_r_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_m_r_w*bmp_img.radar_m_r_h, GFP_KERNEL);
	if (bmp_img.radar_m_r_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_m_r_w,bmp_img.radar_m_r_h);
		return -1;
	}

	bmp_img.radar_l_g_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_l_g_w*bmp_img.radar_l_g_h, GFP_KERNEL);
	if (bmp_img.radar_l_g_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_l_g_w,bmp_img.radar_l_g_h);
		return -1;
	}
	bmp_img.radar_l_o_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_l_o_w*bmp_img.radar_l_o_h, GFP_KERNEL);
	if (bmp_img.radar_l_o_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_l_o_w,bmp_img.radar_l_o_h);
		return -1;
	}
	bmp_img.radar_l_r_src = (char *)kmalloc(bmp_img.rgb_bit*bmp_img.radar_l_r_w*bmp_img.radar_l_r_h, GFP_KERNEL);
	if (bmp_img.radar_l_r_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_l_r_w,bmp_img.radar_l_r_h);
		return -1;
	}

	bmp_img.radar_r_g_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_r_g_w*bmp_img.radar_r_g_h, GFP_KERNEL);
	if (bmp_img.radar_r_g_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_r_g_w,bmp_img.radar_r_g_h);
		return -1;
	}
	bmp_img.radar_r_o_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_r_o_w*bmp_img.radar_r_o_h, GFP_KERNEL);
	if (bmp_img.radar_r_o_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_r_o_w,bmp_img.radar_r_o_h);
		return -1;
	}
	bmp_img.radar_r_r_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_r_r_w*bmp_img.radar_r_r_h, GFP_KERNEL);
	if (bmp_img.radar_r_r_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_r_r_w,bmp_img.radar_r_r_h);
		return -1;
	}
	
	bmp_img.radar_m_g_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_m_g_w*bmp_img.radar_m_g_h, GFP_KERNEL);
	if (bmp_img.radar_m_g_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_m_g_w,bmp_img.radar_m_g_h);
		return -1;
	}
	bmp_img.radar_m_o_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_m_o_w*bmp_img.radar_m_o_h, GFP_KERNEL);
	if (bmp_img.radar_m_o_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_m_o_w,bmp_img.radar_m_o_h);
		return -1;
	}
	bmp_img.radar_m_r_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_m_r_w*bmp_img.radar_m_r_h, GFP_KERNEL);
	if (bmp_img.radar_m_r_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_m_r_w,bmp_img.radar_m_r_h);
		return -1;
	}

	bmp_img.radar_l_g_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_l_g_w*bmp_img.radar_l_g_h, GFP_KERNEL);
	if (bmp_img.radar_l_g_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_l_g_w,bmp_img.radar_l_g_h);
		return -1;
	}
	bmp_img.radar_l_o_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_l_o_w*bmp_img.radar_l_o_h, GFP_KERNEL);
	if (bmp_img.radar_l_o_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_l_o_w,bmp_img.radar_l_o_h);
		return -1;
	}
	bmp_img.radar_l_r_dest = (char *)kmalloc(bmp_img.argb_bit*bmp_img.radar_l_r_w*bmp_img.radar_l_r_h, GFP_KERNEL);
	if (bmp_img.radar_l_r_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_l_r_w,bmp_img.radar_l_r_h);
		return -1;
	}

	printk("%s Exit\r\n",__FUNCTION__);
	return 0;
}
#else
int alloc_memery(void)
{
	/*
	alloc rgb analysis_pictures memery
	alloc argb analysis_pictures memery
	alloc rgb radar_pictures memery
	alloc argb radar_pictures memery
	*/
	
	printk("%s Enter\r\n",__FUNCTION__);
	
	bmp_img.line_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.line_w*bmp_img.line_h, &dma_phy.dma_line_src, GFP_KERNEL);
	if (bmp_img.line_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.line_w,bmp_img.line_h);
		return -1;
	}
	
	bmp_img.line_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.line_w*bmp_img.line_h, &dma_phy.dma_line_dest, GFP_KERNEL);
	if (bmp_img.line_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.line_w,bmp_img.line_h);
		return -1;
	}

	bmp_img.carmodel_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.carmodel_w*bmp_img.carmodel_h, &dma_phy.dma_carmodel_src, GFP_KERNEL);
	if (bmp_img.carmodel_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.carmodel_w,bmp_img.carmodel_h);
		return -1;
	}
	
	bmp_img.carmodel_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.carmodel_w*bmp_img.carmodel_h, &dma_phy.dma_carmodel_dest, GFP_KERNEL);
	if (bmp_img.carmodel_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.carmodel_w,bmp_img.carmodel_h);
		return -1;
	}

	bmp_img.radar_r_g_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_r_g_w*bmp_img.radar_r_g_h, &dma_phy.dma_radar_r_g_src, GFP_KERNEL);
	if (bmp_img.radar_r_g_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_r_g_w,bmp_img.radar_r_g_h);
		return -1;
	}
	bmp_img.radar_r_o_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_r_o_w*bmp_img.radar_r_o_h, &dma_phy.dma_radar_r_o_src, GFP_KERNEL);
	if (bmp_img.radar_r_o_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_r_o_w,bmp_img.radar_r_o_h);
		return -1;
	}
	bmp_img.radar_r_r_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_r_r_w*bmp_img.radar_r_r_h, &dma_phy.dma_radar_r_r_src, GFP_KERNEL);
	if (bmp_img.radar_r_r_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_r_r_w,bmp_img.radar_r_r_h);
		return -1;
	}

	bmp_img.radar_m_g_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_m_g_w*bmp_img.radar_m_g_h, &dma_phy.dma_radar_m_g_src, GFP_KERNEL);
	if (bmp_img.radar_m_g_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_m_g_w,bmp_img.radar_m_g_h);
		return -1;
	}
	bmp_img.radar_m_o_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_m_o_w*bmp_img.radar_m_o_h, &dma_phy.dma_radar_m_o_src, GFP_KERNEL);
	if (bmp_img.radar_m_o_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_m_o_w,bmp_img.radar_m_o_h);
		return -1;
	}
	bmp_img.radar_m_r_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_m_r_w*bmp_img.radar_m_r_h, &dma_phy.dma_radar_m_r_src, GFP_KERNEL);
	if (bmp_img.radar_m_r_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_m_r_w,bmp_img.radar_m_r_h);
		return -1;
	}

	bmp_img.radar_l_g_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_l_g_w*bmp_img.radar_l_g_h, &dma_phy.dma_radar_l_g_src, GFP_KERNEL);
	if (bmp_img.radar_l_g_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_l_g_w,bmp_img.radar_l_g_h);
		return -1;
	}
	bmp_img.radar_l_o_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_l_o_w*bmp_img.radar_l_o_h, &dma_phy.dma_radar_l_o_src, GFP_KERNEL);
	if (bmp_img.radar_l_o_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_l_o_w,bmp_img.radar_l_o_h);
		return -1;
	}
	bmp_img.radar_l_r_src = (char *)dma_alloc_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_l_r_w*bmp_img.radar_l_r_h, &dma_phy.dma_radar_l_r_src, GFP_KERNEL);
	if (bmp_img.radar_l_r_src == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.rgb_bit,bmp_img.radar_l_r_w,bmp_img.radar_l_r_h);
		return -1;
	}

	bmp_img.radar_r_g_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_r_g_w*bmp_img.radar_r_g_h, &dma_phy.dma_radar_r_g_dest, GFP_KERNEL);
	if (bmp_img.radar_r_g_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_r_g_w,bmp_img.radar_r_g_h);
		return -1;
	}
	bmp_img.radar_r_o_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_r_o_w*bmp_img.radar_r_o_h, &dma_phy.dma_radar_r_o_dest, GFP_KERNEL);
	if (bmp_img.radar_r_o_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_r_o_w,bmp_img.radar_r_o_h);
		return -1;
	}
	bmp_img.radar_r_r_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_r_r_w*bmp_img.radar_r_r_h, &dma_phy.dma_radar_r_r_dest, GFP_KERNEL);
	if (bmp_img.radar_r_r_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_r_r_w,bmp_img.radar_r_r_h);
		return -1;
	}
	
	bmp_img.radar_m_g_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_m_g_w*bmp_img.radar_m_g_h, &dma_phy.dma_radar_m_g_dest, GFP_KERNEL);
	if (bmp_img.radar_m_g_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_m_g_w,bmp_img.radar_m_g_h);
		return -1;
	}
	bmp_img.radar_m_o_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_m_o_w*bmp_img.radar_m_o_h, &dma_phy.dma_radar_m_o_dest, GFP_KERNEL);
	if (bmp_img.radar_m_o_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_m_o_w,bmp_img.radar_m_o_h);
		return -1;
	}
	bmp_img.radar_m_r_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_m_r_w*bmp_img.radar_m_r_h, &dma_phy.dma_radar_m_r_dest, GFP_KERNEL);
	if (bmp_img.radar_m_r_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_m_r_w,bmp_img.radar_m_r_h);
		return -1;
	}

	bmp_img.radar_l_g_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_l_g_w*bmp_img.radar_l_g_h, &dma_phy.dma_radar_l_g_dest, GFP_KERNEL);
	if (bmp_img.radar_l_g_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_l_g_w,bmp_img.radar_l_g_h);
		return -1;
	}
	bmp_img.radar_l_o_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_l_o_w*bmp_img.radar_l_o_h, &dma_phy.dma_radar_l_o_dest, GFP_KERNEL);
	if (bmp_img.radar_l_o_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_l_o_w,bmp_img.radar_l_o_h);
		return -1;
	}
	bmp_img.radar_l_r_dest = (char *)dma_alloc_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_l_r_w*bmp_img.radar_l_r_h, &dma_phy.dma_radar_l_r_dest, GFP_KERNEL);
	if (bmp_img.radar_l_r_dest == NULL) {
		printk("ubmp kmalloc %d*%d*%d Bytes memory failed!\n",bmp_img.argb_bit,bmp_img.radar_l_r_w,bmp_img.radar_l_r_h);
		return -1;
	}

	printk("%s Exit\r\n",__FUNCTION__);
	return 0;
}

#endif


#if 0
int free_memery(void)
{
	printk("%s Enter\r\n",__FUNCTION__);
	kfree(bmp_img.line_src);
	kfree(bmp_img.carmodel_src);
	kfree(bmp_img.radar_r_g_src);
	kfree(bmp_img.radar_m_g_src);
	kfree(bmp_img.radar_l_g_src);

	kfree(bmp_img.radar_r_o_src);
	kfree(bmp_img.radar_m_o_src);
	kfree(bmp_img.radar_l_o_src);

	kfree(bmp_img.radar_r_r_src);
	kfree(bmp_img.radar_m_r_src);
	kfree(bmp_img.radar_l_r_src);

	kfree(bmp_img.line_dest);
	kfree(bmp_img.carmodel_dest);
	kfree(bmp_img.radar_r_g_dest);
	kfree(bmp_img.radar_m_g_dest);
	kfree(bmp_img.radar_l_g_dest);

	kfree(bmp_img.radar_r_o_dest);
	kfree(bmp_img.radar_m_o_dest);
	kfree(bmp_img.radar_l_o_dest);

	kfree(bmp_img.radar_r_r_dest);
	kfree(bmp_img.radar_m_r_dest);
	kfree(bmp_img.radar_l_r_dest);

	//kfree(base);

	bmp_img.line_src = NULL;
	bmp_img.carmodel_src = NULL;

	bmp_img.line_dest = NULL;
	bmp_img.carmodel_dest = NULL;
	
	bmp_img.radar_r_g_src = NULL;
	bmp_img.radar_m_g_src = NULL;
	bmp_img.radar_l_g_src = NULL;
	bmp_img.radar_r_o_src = NULL;
	bmp_img.radar_m_o_src = NULL;
	bmp_img.radar_l_o_src = NULL;
	bmp_img.radar_r_r_src = NULL;
	bmp_img.radar_m_r_src = NULL;
	bmp_img.radar_l_r_src = NULL;

	bmp_img.radar_r_g_dest = NULL;
	bmp_img.radar_m_g_dest = NULL;
	bmp_img.radar_l_g_dest = NULL;
	bmp_img.radar_r_o_dest = NULL;
	bmp_img.radar_m_o_dest = NULL;
	bmp_img.radar_l_o_dest = NULL;
	bmp_img.radar_r_r_dest = NULL;
	bmp_img.radar_m_r_dest = NULL;
	bmp_img.radar_l_r_dest = NULL;

	//base = NULL;
	
	printk("%s Exit\r\n",__FUNCTION__);
	return 0;
}
#else
int free_memery(void)
{
	printk("%s Enter\r\n",__FUNCTION__);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.line_w*bmp_img.line_h, bmp_img.line_src, &dma_phy.dma_line_src);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.line_w*bmp_img.line_h, bmp_img.line_dest, &dma_phy.dma_line_dest);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.carmodel_w*bmp_img.carmodel_h, bmp_img.carmodel_src, &dma_phy.dma_carmodel_src);
	
	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.carmodel_w*bmp_img.carmodel_h, bmp_img.carmodel_dest, &dma_phy.dma_carmodel_dest);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_r_g_w*bmp_img.radar_r_g_h, bmp_img.radar_r_g_src, &dma_phy.dma_radar_r_g_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_r_o_w*bmp_img.radar_r_o_h, bmp_img.radar_r_o_src, &dma_phy.dma_radar_r_o_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_r_r_w*bmp_img.radar_r_r_h, bmp_img.radar_r_r_src,&dma_phy.dma_radar_r_r_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_m_g_w*bmp_img.radar_m_g_h, bmp_img.radar_m_g_src,&dma_phy.dma_radar_m_g_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_m_o_w*bmp_img.radar_m_o_h, bmp_img.radar_m_o_src, &dma_phy.dma_radar_m_o_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_m_r_w*bmp_img.radar_m_r_h, bmp_img.radar_m_r_src, &dma_phy.dma_radar_m_r_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_l_g_w*bmp_img.radar_l_g_h, bmp_img.radar_l_g_src, &dma_phy.dma_radar_l_g_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_l_o_w*bmp_img.radar_l_o_h, bmp_img.radar_l_o_src, &dma_phy.dma_radar_l_o_src);

	dma_free_coherent(NULL, bmp_img.rgb_bit*bmp_img.radar_l_r_w*bmp_img.radar_l_r_h, bmp_img.radar_l_r_src, &dma_phy.dma_radar_l_r_src);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_r_g_w*bmp_img.radar_r_g_h, bmp_img.radar_r_g_dest, &dma_phy.dma_radar_r_g_dest);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_r_o_w*bmp_img.radar_r_o_h, bmp_img.radar_r_o_dest, &dma_phy.dma_radar_r_o_dest);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_r_r_w*bmp_img.radar_r_r_h, bmp_img.radar_r_r_dest, &dma_phy.dma_radar_r_r_dest);
	
	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_m_g_w*bmp_img.radar_m_g_h, bmp_img.radar_m_g_dest, &dma_phy.dma_radar_m_g_dest);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_m_o_w*bmp_img.radar_m_o_h, bmp_img.radar_m_o_dest, &dma_phy.dma_radar_m_o_dest);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_m_r_w*bmp_img.radar_m_r_h, bmp_img.radar_m_r_dest, &dma_phy.dma_radar_m_r_dest);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_l_g_w*bmp_img.radar_l_g_h, bmp_img.radar_l_g_dest, &dma_phy.dma_radar_l_g_dest);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_l_o_w*bmp_img.radar_l_o_h, bmp_img.radar_l_o_dest, &dma_phy.dma_radar_l_o_dest);

	dma_free_coherent(NULL, bmp_img.argb_bit*bmp_img.radar_l_r_w*bmp_img.radar_l_r_h, bmp_img.radar_l_r_dest, &dma_phy.dma_radar_l_r_dest);

	//dma_release_channel(chan);
	printk("%s Exit\r\n",__FUNCTION__);
	return 0;
}

#endif

int analysis_auxiliary_line_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.line_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.line_src+11) << 8 | *(bmp_img.line_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.line_src, bmp_img.rgb_bit*bmp_img.line_w*bmp_img.line_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}

int analysis_radar_r_g_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_r_g_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_r_g_src+11) << 8 | *(bmp_img.radar_r_g_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_r_g_src, bmp_img.rgb_bit*bmp_img.radar_r_g_w*bmp_img.radar_r_g_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}

int analysis_radar_r_o_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_r_o_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_r_o_src+11) << 8 | *(bmp_img.radar_r_o_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_r_o_src, bmp_img.rgb_bit*bmp_img.radar_r_o_w*bmp_img.radar_r_o_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}

int analysis_radar_r_r_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_r_r_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_r_r_src+11) << 8 | *(bmp_img.radar_r_r_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_r_r_src, bmp_img.rgb_bit*bmp_img.radar_r_r_w*bmp_img.radar_r_r_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}


int analysis_radar_m_g_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_m_g_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_m_g_src+11) << 8 | *(bmp_img.radar_m_g_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_m_g_src, bmp_img.rgb_bit*bmp_img.radar_m_g_w*bmp_img.radar_m_g_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}

int analysis_radar_m_o_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_m_o_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_m_o_src+11) << 8 | *(bmp_img.radar_m_o_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_m_o_src, bmp_img.rgb_bit*bmp_img.radar_m_o_w*bmp_img.radar_m_o_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}

int analysis_radar_m_r_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_m_r_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_m_r_src+11) << 8 | *(bmp_img.radar_m_r_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_m_r_src, bmp_img.rgb_bit*bmp_img.radar_m_r_w*bmp_img.radar_m_r_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}


int analysis_radar_l_g_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_l_g_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_l_g_src+11) << 8 | *(bmp_img.radar_l_g_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_l_g_src, bmp_img.rgb_bit*bmp_img.radar_l_g_w*bmp_img.radar_l_g_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}

int analysis_radar_l_o_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_l_o_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_l_o_src+11) << 8 | *(bmp_img.radar_l_o_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_l_o_src, bmp_img.rgb_bit*bmp_img.radar_l_o_w*bmp_img.radar_l_o_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}

int analysis_radar_l_r_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_l_r_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.radar_l_r_src+11) << 8 | *(bmp_img.radar_l_r_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);//54	0
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.radar_l_r_src, bmp_img.rgb_bit*bmp_img.radar_l_r_w*bmp_img.radar_l_r_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}


int analysis_carmodel_pictures(const char *path, void *base)
{
	int cnt = 0;
	int start = 0;
	struct file *filp;
	mm_segment_t fs_old;

	loff_t pos = 0;
	
	filp = filp_open(path, O_RDONLY, 0);	
	if (IS_ERR(filp)){
		printk("Open file failed, error code = %d\n", (int)filp);
		return -1;
	}
			
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	cnt = vfs_read(filp, (__force char __user *)bmp_img.carmodel_src, 16, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);
	
	start = *(bmp_img.carmodel_src+11) << 8 | *(bmp_img.carmodel_src+10);
	//printk("start = %d   %d\r\n",start,filp->f_pos);
	filp->f_pos = start;

	cnt = vfs_read(filp, (__force char __user *)bmp_img.carmodel_src, bmp_img.rgb_bit*bmp_img.carmodel_w*bmp_img.carmodel_h, &(filp->f_pos));
	//printk("cnt = %d, filp->f_pos = %d\r\n",cnt, filp->f_pos);

	//printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}




int rgb_to_argb(unsigned char *dest, unsigned char *src, int dest_w, int dest_h)
{
	int w = 0,h = 0;
	int k = 0;
    int tmp = 0;
    int count = 0;
    int c_h = 0, c_w = 0;
	int r_val = 0,g_val = 0,b_val = 0;

#if 0
	printk("dest_w = %d, dest_h = %d\r\n",dest_w, dest_h);
	
	for(h = 0; h < dest_h; h++) {
		for(w = 0; w < dest_w; w++) {
			*(c_h*dest_w*4+4*c_w+dest) = *(h*dest_w*3+3*w+src);
		    *(c_h*dest_w*4+4*c_w+dest+1) = *(h*dest_w*3+3*w+src+1);
			*(c_h*dest_w*4+4*c_w+dest+2) = *(h*dest_w*3+3*w+src+2);
			*(c_h*dest_w*4+4*c_w+dest+3) = 0;
			
			count++;
			c_w++;
			if((count+1) % (dest_w) == 1 && (count!=1)) {
				c_h++;
				c_w = 0;
			}
		}
	}
#endif

#if 1
	//printk("dest_w = %d, dest_h = %d\r\n",dest_w, dest_h);
	for(h = 0; h < dest_h; h++) {
		for(w = 0; w < dest_w; w++) {
			*(c_h*dest_w*4+4*c_w+dest)   = *(h*dest_w*3+3*w+src);    //r
			*(c_h*dest_w*4+4*c_w+dest+1) = *(h*dest_w*3+3*w+src+1);  //g
			*(c_h*dest_w*4+4*c_w+dest+2) = *(h*dest_w*3+3*w+src+2);  //b
			//tmp = *(c_h*dest_w*4+4*c_w+dest) | *(c_h*dest_w*4+4*c_w+dest+1) | *(c_h*dest_w*4+4*c_w+dest+2);
			r_val = *(c_h*dest_w*4+4*c_w+dest);
			g_val = *(c_h*dest_w*4+4*c_w+dest+1);
			b_val = *(c_h*dest_w*4+4*c_w+dest+2);
			if(r_val < 10 && g_val < 10 && b_val < 10) {
				*(c_h*dest_w*4+4*c_w+dest+3) = 0;
			} else {
				*(c_h*dest_w*4+4*c_w+dest+3) = 255;
			}
				
			count++;
			c_w++;
			if((count+1) % (dest_w) == 1 && (count!=1)) {
				c_h++;
				c_w = 0;
			}
		}
	}
#endif

    return 0;	
}


int car_model_rgb_to_argb(unsigned char *dest, unsigned char *src, int dest_w, int dest_h)
{
	int w = 0,h = 0;
	int k = 0;
    int tmp = 0;
    int count = 0;
    int c_h = 0, c_w = 0;

	for(h = 0; h < dest_h; h++) {
		for(w = 0; w < dest_w; w++) {
			*(c_h*dest_w*4+4*c_w+dest)   = *(h*dest_w*3+3*w+src);    //r
			*(c_h*dest_w*4+4*c_w+dest+1) = *(h*dest_w*3+3*w+src+1);  //g
			*(c_h*dest_w*4+4*c_w+dest+2) = *(h*dest_w*3+3*w+src+2);  //b
			*(c_h*dest_w*4+4*c_w+dest+3) = 255;
				
			count++;
			c_w++;
			if((count+1) % (dest_w) == 1 && (count!=1)) {
				c_h++;
				c_w = 0;
			}
		}
	}

    return 0;	
}


int copy_to_base(unsigned char *base, unsigned char *src, int src_w, int src_h, int point_w, int point_h)
{
    int w = 0, h = 0;
	int ret = 0;

	int p_byte = (32 / 8) / 4 * 4;
	int base_w = AUXLAYER_WIDTH, base_h = AUXLAYER_HEIGHT;

	//printk("base = 0x%x, src_w = %d, src_h = %d, p_byte = %d\r\n",base,src_w,src_h,p_byte);
	//printk("src = 0x%x, src_w = %d, src_h = %d, p_byte = %d\r\n",src,src_w,src_h,p_byte);
	//printk("point_w = %d, point_h = %d\r\n",point_w,point_h);
	#if 1 //copy to base
	for(h = 0; h < src_h; h++) {
		for(w = 0; w < src_w * p_byte; w++) {
		  *(base + (h+point_h)*base_w*p_byte + (w+point_w*p_byte)) = *(src + h*src_w*p_byte + w);//pix(600,240)
		}
	}
	#endif

	return 0;
}


int radar_copy_to_base(unsigned char *base, unsigned char *src, int src_w, int src_h, int point_w, int point_h)
{
    int w = 0, h = 0;
	int ret = 0;

	int p_byte = (32 / 8) / 4 * 4;
	int base_w = AUXLAYER_WIDTH, base_h = AUXLAYER_HEIGHT;
	int r_val = 0,g_val = 0,b_val = 0;

	for(h = 0; h < src_h; h++) {
		for(w = 0; w < src_w * p_byte; w++) {
			r_val = *(h*src_w*4+w+src);
			g_val = *(h*src_w*4+w+src+1);
			b_val = *(h*src_w*4+w+src+2);
			if(r_val > 5 || g_val > 5 || b_val > 5) {
		    	*(base + (h+point_h)*base_w*p_byte + (w+point_w*p_byte)) = *(src + h*src_w*p_byte + w);//pix(600,240)
			}
		}
	}

	return 0;
}


int print_pic_data(unsigned char *src, int p_byte, int src_w, int src_h)
{
	int w = 0, h = 0;
	
	printk("src = 0x%x,p_byte = %d,src_w = %d,src_h = %d\r\n",src,p_byte,src_w,src_h);
	printk("\n\n");
    for(h = 0; h < src_h; h++) {
        for(w = 0; w < src_w*p_byte; w++) {
			printk("%4x",*((h*src_w*p_byte+w)+src));
        }
        printk("\n");
    }
	
	printk("\n");
	return 0;
}


int copy_to_dest(unsigned char *dest, unsigned char *src, int dest_w, int dest_h, int src_w, int src_h, int point_w, int point_h)
{
	int w = 0, h = 0;
	int p_byte = 3;
	//dest_w = 720, dest_h = 480, src_w = 110, src_h = 120;
	
    #if 1  
	for(h = 0; h < src_h; h++) {
		for(w = 0; w < src_w * p_byte; w++) {
		  *(dest + (h+point_h)*dest_w*p_byte + (w+point_w*p_byte)) = *(src + h*src_w*p_byte + w);//pix(100,100)
		}
	}
	#endif

	//printk("%s\r\n",__FUNCTION__);
	return 0;
}

#if 0
int clear_cache(void *data,int data_w, int data_h, int p_byte)
{
	void *start, *end;
	
	start = data;
	end = (void *)((unsigned long)start + data_w * data_h * p_byte);

	memset(data, 0, data_w * data_h * p_byte);
	dmac_flush_range(start, end);

	return 0;
}
#endif

int analysis_of_radar_pictures(const char *bmp_path, const char *radar_path, void *base)
{
	int ap_byte = 3;
	int bp_byte = (32 / 8) / 4 * 4;
	int bmp_dest_w = 720, bmp_dest_h = 480;
	int radar_dest_w = 120, radar_dest_h = 120;
	int cnt = 0;

	char *bmp_dest;
	char *radar_dest;
	char *bmp_header;
	char *radar_header;	
	int start = 0;
	
	struct file *bmp_filp;
	struct file *radar_filp;
	mm_segment_t fs_old;
	loff_t pos = 0;
	
	bmp_filp = filp_open(bmp_path, O_RDONLY, 0);	
	if (IS_ERR(bmp_filp)){
		printk("Open file failed, error code = %d\n", (int)bmp_filp);
		return -1;
	}
	
	radar_filp = filp_open(radar_path, O_RDONLY, 0);	
	if (IS_ERR(radar_filp)){
		printk("Open file failed, error code = %d\n", (int)radar_filp);
		return -1;
	}		
		
	fs_old = get_fs();	
	set_fs(KERNEL_DS);

	bmp_header = (char *)kmalloc(ap_byte*bmp_dest_w*bmp_dest_h, GFP_KERNEL);
	if (bmp_header == NULL) {
		printk("upng kmalloc %d Bytes memory failed!\n",ap_byte*bmp_dest_w*bmp_dest_h);
		filp_close(bmp_filp, NULL);
		return -1;
	}	

	radar_header = (char *)kmalloc(ap_byte*radar_dest_w*radar_dest_h, GFP_KERNEL);
	if (radar_header == NULL) {
		printk("upng kmalloc %d Bytes memory failed!\n",ap_byte*radar_dest_w*radar_dest_h);
		filp_close(radar_filp, NULL);
		return -1;
	}	

	bmp_dest = (char *)kmalloc(bp_byte*bmp_dest_w*bmp_dest_h, GFP_KERNEL);

	if (bmp_dest == NULL) {
		printk("kmalloc %d Bytes memory failed!\n",bp_byte*bmp_dest_w*bmp_dest_h);
		return -1;
	}


	radar_dest = (char *)kmalloc(bp_byte*radar_dest_w*radar_dest_h, GFP_KERNEL);

	if (radar_dest == NULL) {
		printk("kmalloc %d Bytes memory failed!\n",bp_byte*radar_dest_w*radar_dest_h);
		return -1;
	}
	
	cnt = vfs_read(radar_filp, (__force char __user *)radar_header, 16, &(radar_filp->f_pos));
	printk("cnt = %d, radar_filp->f_pos = %d\r\n",cnt, radar_filp->f_pos);
	
	start = *(radar_header+11) << 8 | *(radar_header+10);
	printk("start = %d   %d\r\n",start,radar_filp->f_pos);//54	0
	radar_filp->f_pos = start;
	bmp_filp->f_pos =start;

	cnt = vfs_read(bmp_filp, (__force char __user *)bmp_header, ap_byte*bmp_dest_w*bmp_dest_h, &(bmp_filp->f_pos));
	printk("cnt = %d, bmp_filp->f_pos = %d\r\n",cnt, bmp_filp->f_pos);
	cnt = vfs_read(radar_filp, (__force char __user *)radar_header, ap_byte*radar_dest_w*radar_dest_h, &(radar_filp->f_pos));
	printk("cnt = %d, radar_filp->f_pos = %d\r\n",cnt, radar_filp->f_pos);

    //print_pic_data(radar_header, ap_byte, radar_dest_w, radar_dest_h);
	rgb_to_argb(bmp_dest, bmp_header, bmp_dest_w, bmp_dest_h);//dest is argb
	rgb_to_argb(radar_dest, radar_header, radar_dest_w, radar_dest_h);//dest is argb

	//print_pic_data(radar_dest, bp_byte, radar_dest_w, radar_dest_h);
	
	copy_to_base(base, bmp_dest, bmp_dest_w, bmp_dest_h, 600, 240);
	copy_to_base(base, radar_dest, radar_dest_w, radar_dest_h, 100, 240);

	//print_pic_data(base, bp_byte, base_w, base_h);

	//copy_to_dest(bmp_header,radar_header);

	printk("\n\n\n %s\r\n",__FUNCTION__);
 
	filp_close(bmp_filp, NULL);
	filp_close(radar_filp, NULL);
	set_fs(fs_old);
 
	return 0;   
}
