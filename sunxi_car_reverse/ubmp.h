/*
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.

		2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		distribution.
*/

#if !defined(UBMP_H)
#define UBMP_H

#define AUXLAYER_WIDTH (1920)
#define AUXLAYER_HEIGHT (720)
#define AUXLAYER_SIZE (AUXLAYER_WIDTH * AUXLAYER_HEIGHT * 4)

typedef unsigned char BYTE ;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;

typedef struct BMPFileHead_t {
    WORD bfType;          /* 说明文件的类型 */
    DWORD bfSize;         /* 说明文件的大小，用字节为单位 */
    WORD bfReserved1;     /* 保留，设置为0 */
    WORD bfReserved2;     /* 保留，设置为0 */
    DWORD bfOffsetBytes;  /* 说明从BITMAPFILEHEADER结构开始到实际的图像数据之间的字节偏移量 */
}BMPFileHead;             //14字节,但是sizeof计算长度时为16字节


typedef struct BMPHeaderInfo_t {
    DWORD biSize;           /* 说明结构体所需字节数 */
    LONG biWidth;           /* 以像素为单位说明图像的宽度 */
    LONG biHeight;          /* 以像素为单位说明图像的高度 */
    WORD biPlanes;          /* 说明位面数，必须为1 */
    WORD biBitCount;        /* 说明位数/像素，1、2、4、8、24 */
    DWORD biCompression;    /* 说明图像是否压缩及压缩类型BI_RGB，BI_RLE8，BI_RLE4，BI_BITFIELDS */
    DWORD biSizeImage;      /* 以字节为单位说明图像大小，必须是4的整数倍*/
    LONG biXPixelsPerMeter; /*目标设备的水平分辨率，像素/米 */
    LONG biYPixelsPerMeter; /*目标设备的垂直分辨率，像素/米 */
    DWORD biClrUsed;        /* 说明图像实际用到的颜色数，如果为0，则颜色数为2的biBitCount次方 */
    DWORD biClrImportant;   /*说明对图像显示有重要影响的颜色索引的数目，如果是0，表示都重要。*/
}BMPHeaderInfo;             //40字节


typedef struct  RGB_t{
    BYTE rgbBlue;     /*指定蓝色分量*/
    BYTE rgbGreen;    /*指定绿色分量*/
    BYTE rgbRed;      /*指定红色分量*/
    BYTE rgbReserved; /*保留，指定为0*/
}RGB;

/*
typedef struct serial_s{
	char type[16];//0:camera 1:radar_r 2:radar_m 3:radar_l
	int idx;//0:camera 1:radar_r 2:radar_m 3:radar_l
} serial_t;

typedef struct sensor_s{
	char type[16];//0:camera 1:radar_r 2:radar_m 3:radar_l
	int idx;//0:camera 1:radar_r 2:radar_m 3:radar_l
	int status;
} sensor_t;

sensor_t sensor;
*/

typedef struct bmp_img_s{
    unsigned char *line_dest;
    unsigned char *line_src;
    
    unsigned char *carmodel_dest;
    unsigned char *carmodel_src;
	
    unsigned char *radar_dest;
	
    unsigned char *radar_r_g_dest;
    unsigned char *radar_m_g_dest;
    unsigned char *radar_l_g_dest;
    unsigned char *radar_r_o_dest;
    unsigned char *radar_m_o_dest;
    unsigned char *radar_l_o_dest;
    unsigned char *radar_r_r_dest;
    unsigned char *radar_m_r_dest;
    unsigned char *radar_l_r_dest;

    unsigned char *radar_src;	
	
    unsigned char *radar_r_g_src;
    unsigned char *radar_m_g_src;
    unsigned char *radar_l_g_src;
    unsigned char *radar_r_o_src;
    unsigned char *radar_m_o_src;
    unsigned char *radar_l_o_src;
    unsigned char *radar_r_r_src;
    unsigned char *radar_m_r_src;
    unsigned char *radar_l_r_src;

    unsigned char line_path[32];
    unsigned char carmodel_path[32];	
    unsigned char radar_r_path[32];
    unsigned char radar_m_path[32];
    unsigned char radar_l_path[32];

    int line_idx;
    int carmodel_idx;
	
    int radar_r_idx;
    int radar_m_idx;
    int radar_l_idx;
	
    int line_h;
    int line_w;
    
    int carmodel_h;
    int carmodel_w;

    int radar_r_g_h;
    int radar_m_g_h;
    int radar_l_g_h;  
    int radar_r_o_h;
    int radar_m_o_h;
    int radar_l_o_h; 
    int radar_r_r_h;
    int radar_m_r_h;
    int radar_l_r_h; 

    int radar_r_g_w;
    int radar_m_g_w;
    int radar_l_g_w;  
    int radar_r_o_w;
    int radar_m_o_w;
    int radar_l_o_w; 
    int radar_r_r_w;
    int radar_m_r_w;
    int radar_l_r_w; 
  
    int rgb_bit;
    int argb_bit;
} bmp_img_t;


int init_picture(void);
int alloc_memery(void); 
int free_memery(void);
//int clear_cache(int screen_id, void *data,int data_w, int data_h, int p_byte);

int	analysis_auxiliary_line_pictures(const char *path, void *base);
int analysis_carmodel_pictures(const char *path, void *base);
int	analysis_radar_r_g_pictures(const char *path, void *base);
int	analysis_radar_m_g_pictures(const char *path, void *base);
int	analysis_radar_l_g_pictures(const char *path, void *base);
int	analysis_radar_r_o_pictures(const char *path, void *base);
int	analysis_radar_m_o_pictures(const char *path, void *base);
int	analysis_radar_l_o_pictures(const char *path, void *base);
int	analysis_radar_r_r_pictures(const char *path, void *base);
int	analysis_radar_m_r_pictures(const char *path, void *base);
int	analysis_radar_l_r_pictures(const char *path, void *base);


int copy_to_base(unsigned char *base, unsigned char *src, int src_w, int src_h, int point_w, int point_h);
int radar_copy_to_base(unsigned char *base, unsigned char *src, int src_w, int src_h, int point_w, int point_h);
int copy_to_dest(unsigned char *dest, unsigned char *src, int dest_w, int dest_h, int src_w, int src_h, int point_w, int point_h);
int rgb_to_argb(unsigned char *dest, unsigned char *src, int dest_w, int dest_h);
int car_model_rgb_to_argb(unsigned char *dest, unsigned char *src, int dest_w, int dest_h);

#endif /*defined(UBMP_H)*/
