#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define REGFLAG_DELAY                                   0XFE
#define REGFLAG_END_OF_TABLE                            0xFF   // END OF REGISTERS MARKER
#define I2C_NUM                                         2
#define I2C_ADDR                                        0x0c

typedef struct {
    unsigned char cmd;
    unsigned char data;
} ti94x_setting_table;

static ti94x_setting_table ti94x_init_table[] = {
        {0x01, 0x08},//reset
        {0x1e, 0x01},
        {0x32, 0x80},
        {0x33, 0x07},
        {0x36, 0x00},
        {0x37, 0x80},
        {0x38, 0x7f},
        {0x39, 0x07},
        {0x3A, 0x00},
        {0x3B, 0x00},
        {0x3C, 0xCF},
        {0x3D, 0x02},
        {0x4F, 0x0C},
        {0x56, 0x00},
        {0x64, 0xF0},
        /*share register conf*/
        {0x03, 0xDA},
        {0x04, 0x10},//30
        {0x5B, 0x00},
        /*port0 conf*/
        //0x06, 0x58,//2C
        {0x07, 0x34},//1A
        {0x08, 0x34},//1A
        {0x0E, 0x30},
        {0x0F, 0x03},
        {0x40, 0x04},
        {0x41, 0x05},
        {0x42, 0x18},
        {0x41, 0x20},
        {0x42, 0x6f},
        {REGFLAG_DELAY,5},
        {0x01, 0x00},
        {0x17, 0x9E},
        {0x5B, 0x23},
        {0x0F, 0x03},
        {0x0D, 0x03},
        {0x0E, 0x33},
        {REGFLAG_END_OF_TABLE, 0x00}
};

static void ti94x_write_byte(unsigned char cmd,unsigned char data)
{
	int num = 0;
	unsigned char addr,reg,val = 0;
	
	char chars[100];
	num = I2C_NUM;
	addr = I2C_ADDR;
	reg = cmd;
	val = data;
	sprintf(chars,"i2cset -y -f %d %d %d %d",num,addr,reg,val);
	system(chars);
}

static void ti94x_read_byte(unsigned char cmd,unsigned char *data)
{
	int num = 0;
	unsigned char addr,reg = 0;
	unsigned char *val = 0;
	char chars[100];
	int ret = 0;
	
	num = I2C_NUM;
	addr = I2C_ADDR;
	reg = cmd;
	val = data;
	sprintf(chars,"i2cget -y -f %d %d %d",num,addr,reg);
	ret = system(chars);
	*data = (char)ret;
	val = data;
}

static void i2c_dump(void)
{
	int num = 0;
	unsigned char addr = 0;
	char chars[100];

	num = I2C_NUM;
	addr = I2C_ADDR;
	sprintf(chars,"i2cdump -y -f %d %d",num,addr);
	system(chars);	
}

static void push_table(ti94x_setting_table *table, unsigned int count)
{
    unsigned int i;

    for(i = 0; i < count; i++) {

        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                usleep(table[i].data*1000);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                ti94x_write_byte(cmd, table[i].data);
        }
    }
}

static void dump_reg_table(ti94x_setting_table *table, unsigned int count)
{
    unsigned int i;
    unsigned char data;

    for(i = 0; i < count; i++) {

        unsigned cmd;
        cmd = table[i].cmd;
        switch (cmd) {
            case REGFLAG_DELAY :
            usleep(table[i].data*1000);
                break;
            case 0xFF:
                break;

            default:
                ti94x_read_byte(cmd,&data);
                printf("dump_reg_table ti94x_lk dump cmd=0x%x  data=0x%x \n",cmd,data);
        }
    }
}

enum cmd_type {
    I2C_SET = 1,
	I2C_GET,
	I2C_DUMP,
};

enum parameter_type {
	PT_PROGRAM_NAME = 0,
    CMD_INFO,
    PT_NUM
};

void usage(void)
{
    printf("you should input as:\n");
    printf("\t Ci2cset [cmd]\n");
	printf("\t [1] i2c set all reg\n");
	printf("\t [2] i2c get all reg\n");
	printf("\t [3] i2c dump all reg\n");
}

int main(int argc, char **argv)
{
	unsigned int cmd = 0;
	
    if(argc != PT_NUM) {
        usage();
        return -1;
    }
	
	cmd = atoi(argv[CMD_INFO]);

	switch(cmd)
	{
		case I2C_SET:
			push_table(ti94x_init_table, sizeof(ti94x_init_table)/sizeof(ti94x_setting_table));
		break;

		case I2C_GET:
			dump_reg_table(ti94x_init_table, sizeof(ti94x_init_table)/sizeof(ti94x_setting_table));
	    break;
		
		case I2C_DUMP:
			i2c_dump();
		break;
			
		default:
			printf("need correct cmd!\n");
		return -1;
	} 

	return 0;
}