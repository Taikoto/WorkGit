#include "testitem.h"

#define MAX_NAND_SECTION_NUM 10

int test_emmc_start(void)
{
    struct statfs fs;
    int fd;
    int ret = RL_FAIL; //fail
    int cur_row = 2;
    char buffer[64]={0},temp[128]={0};
    int read_len = 0;
    float size = 0;
    char secPath[128] = {0};
    char *endptr;
    int i = 0;

    ui_fill_locked();
    ui_show_title(MENU_TEST_EMMC);
    gr_flip();
    if (0 == access("/sys/devices/virtual/mtd/",F_OK))			//for nand project(pikel4+2)
    {
        snprintf(temp,sizeof(temp),"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_OK);
        ui_set_color(CL_GREEN);
        cur_row = ui_show_text(cur_row, 0, temp);
        gr_flip();

        for(i = 0; i < MAX_NAND_SECTION_NUM; i++) {
            memset(secPath, 0, sizeof(secPath));
            sprintf(secPath, "/sys/devices/virtual/mtd/mtd%d/size", i);
            fd = open(secPath, O_RDONLY);
            if(fd >= 0)
            {
                read_len = read(fd, buffer,sizeof(buffer));
                close(fd);
                if(read_len > 0)
                {
                    size += strtoul(buffer,&endptr, 0);
                    LOGD("/sys/devices/virtual/mtd/mtd%d/size value = %f, read_len = %d ", i, size, read_len);
                }
            }
            else
                LOGE("fd = %d, open fail", fd);
        }



        snprintf(temp,sizeof(temp), "%s %4.2f MB", TEXT_EMMC_CAPACITY,(size/1024/1024));
        cur_row = ui_show_text(cur_row, 0, temp);
        gr_flip();
        ret = RL_PASS;
    }
    else if (0 == access("/sys/block/mmcblk0",F_OK))
    {
        snprintf(temp,sizeof(temp),"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_OK);
        ui_set_color(CL_GREEN);
        cur_row = ui_show_text(cur_row, 0, temp);
        gr_flip();
        fd = open("/sys/block/mmcblk0/size",O_RDONLY);
        if(fd < 0){
            goto TEST_END;
        }
        read_len = read(fd,buffer,sizeof(buffer));
        close(fd);
        if(read_len <= 0){
            goto TEST_END;
        }

        size = strtoul(buffer,&endptr,0);
        LOGD("sys/block/mmcblk0/size value = %f, read_len = %d ", size, read_len);
        snprintf(temp,sizeof(temp), "%s %4.2f GB", TEXT_EMMC_CAPACITY,(size/2/1024/1024));
        cur_row = ui_show_text(cur_row, 0, temp);
        gr_flip();
        ret = RL_PASS;
    }
    else{
        snprintf(temp,sizeof(temp),"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_FAIL);
        ui_set_color(CL_RED);
        cur_row = ui_show_text(cur_row, 0, temp);
        gr_flip();
	}

TEST_END:
    if(ret == RL_PASS) {
        ui_set_color(CL_GREEN);
        cur_row = ui_show_text(cur_row, 0, TEXT_TEST_PASS);
    } else {
        ui_set_color(CL_RED);
        cur_row = ui_show_text(cur_row, 0, TEXT_TEST_FAIL);
    }
    gr_flip();
    sleep(1);

    save_result(CASE_TEST_EMMC,ret);
    return ret;
}