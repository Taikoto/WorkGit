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

#define CONFIG_FILE "/sdcard/regmap.txt"

typedef struct {
    unsigned char cmd;
    unsigned char data;
} setting_table;
setting_table *ctable;

typedef struct {
    int num;
	unsigned char addr;
	unsigned char reg;
	unsigned char val;
} i2cconfig;
i2cconfig ci2c;

int table_len = 0;

static setting_table init_table[] = {
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

static void ti94x_write_byte(i2cconfig i2c,unsigned char cmd,unsigned char data)
{
	char chars[100];
	//i2c.num = I2C_NUM;
	//i2c.addr = I2C_ADDR;
	i2c.reg = cmd;
	i2c.val = data;
	sprintf(chars,"i2cset -y -f %d %d %d %d",i2c.num,i2c.addr,i2c.reg,i2c.val);
	system(chars);
}

static void ti94x_read_byte(i2cconfig i2c,unsigned char cmd,unsigned char *data)
{
	//unsigned char *val = 0;
	char chars[100];
	int ret = 0;
	
	//i2c.num = I2C_NUM;
	//i2c.addr = I2C_ADDR;
	i2c.reg = cmd;
	//i2c.val = data;
	sprintf(chars,"i2cget -y -f %d %d %d",i2c.num,i2c.addr,i2c.reg);
	ret = system(chars);
	*data = (char)ret;
}

static void i2c_dump(i2cconfig i2c)
{
	char chars[100];

	//i2c.num = I2C_NUM;
	//i2c.addr = I2C_ADDR;
	sprintf(chars,"i2cdump -y -f %d %d",i2c.num,i2c.addr);
	system(chars);	
}

static void push_table(setting_table *table, unsigned int count)
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
                ti94x_write_byte(ci2c,cmd, table[i].data);
        }
    }
}

static void dump_reg_table(setting_table *table, unsigned int count)
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
                ti94x_read_byte(ci2c,cmd,&data);
                printf("dump_reg_table ti94x_lk dump cmd=0x%x  data=0x%x \n",cmd,data);
        }
    }
}

static unsigned char ascii2hex(unsigned char a)
{
	char value = 0;

	if (a >= '0' && a <= '9')
		value = a - '0';
	else if (a >= 'A' && a <= 'F')
		value = a - 'A' + 0x0A;
	else if (a >= 'a' && a <= 'f')
		value = a - 'a' + 0x0A;
	else
		value = 0xff;

	return value;
}

static unsigned char str_to_hex(const unsigned char *src_buf)
{
	int i;
	unsigned char high, low;
	int src_len = 4;

	for(i = 0; i < src_len; i++)
	{
		if((src_buf[i] == '0') && ((src_buf[i + 1] == 'x') || (src_buf[i + 1] == 'X')))
		{
			high = ascii2hex(src_buf[i + 2]);
			low = ascii2hex(src_buf[i + 3]);
			if ((high == 0xFF) || (low == 0xFF))
			{
				return -1;
			}
		}
	}

	return (high << 4) + low;
}

static i2cconfig get_i2cconfig_struct(i2cconfig i2c)
{
	unsigned char str[12] = "";
    int found_num = 0;
    FILE * fp = NULL;
    char filename[64] = {0};
    char line[128] = {0};

    sprintf(filename,CONFIG_FILE);

    if((fp = fopen(filename, "r")))
    {
        while(!feof(fp))// is not blank space
        {
            if(NULL == fgets(line, sizeof(line), fp))
            {
                /* Error or EOF */
                break;
            }

            if(found_num)//first to find num, second to find addr
            {
                if(strstr(line,"i2c_addr:"))
                {
                    sscanf(line, "%*s %s", str);
					printf("str %s\n",str);
					i2c.addr = str_to_hex(str);
                    break;
                }
                else
                {
                    continue;
                }
            }
            if(strstr(line,"i2c_bus_num:"))
            {
				sscanf(line, "%*s %d", &i2c.num);
                found_num = 1;
            }
        }

        fclose(fp);
    }

    printf("num is %d, addr is 0x%x\n",i2c.num,i2c.addr);
	return i2c;
}

static setting_table *get_setting_table(setting_table *table)
{
    unsigned char reg[12] = {0};
	unsigned char val[12] = {0};
    FILE * fp = NULL;
    char filename[64] = {0};
    char line[128] = {0};
	int i = 0;
	int len = 0;

    sprintf(filename,CONFIG_FILE);

    if((fp = fopen(filename, "r")))
    {
        while(!feof(fp))// is not blank space
        {
            if(NULL == fgets(line, sizeof(line), fp))//line is not blank
            {
                /* Error or EOF */
                break;
            }

            if(strstr(line,"static setting_table init_table[] = {"))
			{
				while(1)
				{
					if(NULL == fgets(line, sizeof(line), fp))//line is not blank
					{
						/* Error or EOF */
						break;
					}

					sscanf(line, "%*s %s", reg);
					sscanf((char *)reg, "%[^,]", reg);
					sscanf(line, "%*s %*s %s", val);
					sscanf((char *)val, "%[^}]", val);
					printf("reg %s\n",reg);
					printf("val %s\n",val);
					table[i].cmd = str_to_hex(reg);
					table[i].data = str_to_hex(val);
					i++;
					if(i > 128)
					{
						printf("config file exist error data\n");
						abort();
					}

					if(strstr(line,"};"))
					{
						printf("line %d\n",i);
						len = i - 1;
						table_len = len;
						goto end_data;
					}
				}
			}
        }

        fclose(fp);
    }

end_data:
	printf("get setting_table:\n");
	for(i = 0; i < len; i++)
	{
		printf("{0x%x,0x%x}\n",table[i].cmd,table[i].data);
	}

	return table;
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
	int ret = 0;
	int i = 0;
	int fp = 0;
	int configfile_status = 0;
	unsigned int cmd = 0;
	i2cconfig i2c = {0};
	setting_table *table;

	ci2c.num = 2;
	ci2c.addr = 0x0c;
	
    if(argc != PT_NUM) {
        usage();
        return -1;
    }
	
	cmd = atoi(argv[CMD_INFO]);

	if((ret = (fp = open(CONFIG_FILE, O_RDWR))) < 0) {
		printf("config file doesn't exit,will use default config = %d\n", ret);
		configfile_status = 0;
	} else {
		//1. get_i2cconfig_struct 2. get_setting_table
		table = (void *)malloc(table_len);
		ctable = (void *)malloc(table_len);

		ci2c = get_i2cconfig_struct(i2c);
		printf("num is %d, addr is 0x%x\n",ci2c.num,ci2c.addr);
		//ci2c.num = i2c.num;
		//ci2c.addr = i2c.addr;
		ctable = get_setting_table(table);
		printf("get_setting_table:\n");
		for(i = 0; i < table_len; i++)
		{
			printf("{0x%x,0x%x}\n",ctable[i].cmd,ctable[i].data);
		}
	}

	switch(cmd)
	{
		case I2C_SET:
			if(configfile_status)
				push_table(ctable, sizeof(ctable)/sizeof(setting_table));
			else
				push_table(init_table, sizeof(init_table)/sizeof(setting_table));
		break;

		case I2C_GET:
			if(configfile_status)
				dump_reg_table(ctable, sizeof(ctable)/sizeof(setting_table));
			else
				dump_reg_table(init_table, sizeof(init_table)/sizeof(setting_table));
	    break;
		
		case I2C_DUMP:
			i2c_dump(ci2c);
		break;
			
		default:
			printf("need correct cmd!\n");
		return -1;
	} 

	return 0;
}