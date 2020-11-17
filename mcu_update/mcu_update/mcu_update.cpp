#include <stdio.h>
#include "../libmcu_update/ib_mcuctl.h"

static const char *BACKCONF_MCU_UPDATE = "/sdcard/2020M_MB_UPDATE.hex";


static int verify_mcu_image(const char *path)
{
    printf("verify_mcu_image\n");
    FILE* read_file = NULL;
    read_file = fopen(path,"rb");

    if (read_file != NULL)
    {
        printf("verify_mcu_image file open success\n");
        fclose(read_file);
        return 1;
    }

    printf("verify_mcu_image file open fail\n");

    return 0;
}


int main(int argc, char **argv)
{
    printf("IBMCUControl start\n");
    IBMCUControl * mcucontrl = IBMCUControl::getInstance();

    if(verify_mcu_image(BACKCONF_MCU_UPDATE) == 1)
    {
       printf("mcu update start.....\n");
       mcucontrl->Upgrade(BACKCONF_MCU_UPDATE);
    }
        
    mcucontrl->reset();
}
