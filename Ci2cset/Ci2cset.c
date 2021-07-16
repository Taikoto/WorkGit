#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define REGFLAG_DELAY                                   0XFE
#define REGFLAG_END_OF_TABLE                            0xFF   // END OF REGISTERS MARKER
#define I2C_NUM                                         2
#define I2C_ADDR                                        0x0c

#define CONFIG_FILE "/sdcard/regmap.txt"

typedef struct {
    unsigned int cmd;
    unsigned char data;
} setting_table;
setting_table *ctable;

typedef struct {
    int num;
	unsigned char addr;
	unsigned int reg;
	unsigned char val;
} i2cconfig;
i2cconfig ci2c;

int table_len = 0;
#if 0
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
#else
static setting_table init_table[] = {
	{ 0x0313, 0x00 },
	{ 0x0006, 0x1f },
	{ 0x0b06, 0xef },
	{ 0x0006, 0x1f },
	{ 0x0b07, 0x08 },
	{ 0x0010, 0x31 },
	{ 0x040b, 0x07 },
	{ 0x042d, 0x15 },
	{ 0x040d, 0x24 },
	{ 0x040e, 0x24 },
	{ 0x040f, 0x00 },
	{ 0x0410, 0x00 },
	{ 0x0411, 0x01 },
	{ 0x0412, 0x01 },
	{ 0x0330, 0x04 },
	{ 0x044a, 0xd0 },
	{ 0x0320, 0x2a },
	{ 0x0313, 0x88 },
	{ 0x0316, 0xa4 },
	{ 0x0317, 0x04 },
	{ 0x031d, 0x6f },
	{ 0x0f00, 0x01 },
	{ 0x0b0f, 0x09 },
	{ 0x0b96, 0x83 },
	{ 0x0ba7, 0x45 },
	{ 0x0313, 0xc2 }
};
#endif

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

static void i2c_transfer_get(i2cconfig i2c,unsigned int cmd,unsigned char *data)
{
	char chars[100],reg_h,reg_l;
	int ret = 0;

	i2c.reg = cmd;

	reg_h = (char)(cmd >> 8);
	reg_l = (char)(cmd & 0xFF);

	//printf("[H L]%x %x\n",reg_h,reg_l);
	sprintf(chars,"i2ctransfer -y -f %d w2@%d %d %d r1",i2c.num,i2c.addr,reg_h,reg_l);
	system(chars);
	ret = system(chars);
	*data = (char)ret;
}


static void i2c_transfer_set(i2cconfig i2c,unsigned int cmd,unsigned char data)
{
	char chars[100],reg_h,reg_l;
	int ret = 0;

	i2c.reg = cmd;
	i2c.val = data;

	reg_h = (char)(cmd >> 8);
	reg_l = (char)(cmd & 0xFF);

	//printf("[H L]%x %x\n",reg_h,reg_l);
	sprintf(chars,"i2ctransfer -y -f %d w3@%d %d %d %d",i2c.num,i2c.addr,reg_h,reg_l,i2c.val);
	system(chars);
	ret = system(chars);
	data = (char)ret;
}

static void push_16b_table(setting_table *table, unsigned int count)
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
                i2c_transfer_set(ci2c,cmd,table[i].data);
        }
    }
}

static void dump_16b_reg_table(setting_table *table, unsigned int count)
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
				printf("dump_16b_reg_table dump cmd=0x%x\n",cmd);
                i2c_transfer_get(ci2c,cmd,&data);
        }
    }
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

int my_stricmp(char *a, char *b)
{
	unsigned char i = 0;
	char ret = 0;

	for (i = 0; i < strlen(a); i++) {
		ret = (char)tolower(a[i]);
		a[i] = ret;
		printf("a[%d] = %c %c\n",i,a[i],ret);
	}

	for (i = 0; i < strlen(b); i++) {
		ret = (char)tolower(b[i]);
		//b[i] = ret;
		printf("b[%d] = %c %c\n",i,b[i],ret);
	}

	ret = strcmp(a,b); //why return strcmp(a,b) is fault?
	printf("ret = %d, %s, %s\n",ret,a,b);
	return ret;
}

static unsigned int str_to_16hex(const unsigned char *src_buf)
{
	int i;
	unsigned char high,high1,high2, low,low1,low2;
	int src_len = 6;

	for(i = 0; i < src_len; i++)
	{
		if((src_buf[i] == '0') && ((src_buf[i + 1] == 'x') || (src_buf[i + 1] == 'X')))
		{
			high1 = ascii2hex(src_buf[i + 2]);
			high2 = ascii2hex(src_buf[i + 3]);
			low1 = ascii2hex(src_buf[i + 4]);
			low2 = ascii2hex(src_buf[i + 5]);
			high = (high1 << 4) + high2;
			low = (low1 << 4) + low2;
			if ((high == 0xFFFF) || (low == 0xFFFF))
			{
				return -1;
			}
		}
	}

	//printf("0x%x %x %x\n",(high << 8) + low, high, low);
	return (high << 8) + low;
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
	char *preg,*pval;
	char *pbuf = "0xFF";
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
					preg = (char *)reg;
					pval = (char *)val;
					printf("my_stricmp = %d %d\n",my_stricmp(preg,pbuf),my_stricmp(pval,pbuf));
					if (0/*my_stricmp(preg,pbuf) < 0*/) { //strimp
						table[i].cmd = str_to_hex(reg);
					} else {
						table[i].cmd = str_to_16hex(reg);
					}
					if (0/*my_stricmp(pval,pbuf) < 0*/) {
						table[i].data = str_to_hex(val);
					} else {
						table[i].data = str_to_16hex(val);
					}
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
	I2C_16B_DUMP,
	I2C_16B_SET,
	I2C_16B_GET,
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
	printf("\t [4] i2c 16b dump all reg\n");
	printf("\t [5] i2c set 16b all reg\n");
	printf("\t [6] i2c get 16b all reg\n");
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
		configfile_status = 1;
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

		case I2C_16B_DUMP:
			if(configfile_status)
				dump_16b_reg_table(ctable, sizeof(ctable)/sizeof(setting_table));
			else
				dump_16b_reg_table(init_table, sizeof(init_table)/sizeof(setting_table));
		break;

		case I2C_16B_SET:
			if(configfile_status)
				push_16b_table(ctable, sizeof(ctable)/sizeof(setting_table));
			else
				push_16b_table(init_table, sizeof(init_table)/sizeof(setting_table));
		break;

		case I2C_16B_GET:
			if(configfile_status)
				dump_16b_reg_table(ctable, sizeof(ctable)/sizeof(setting_table));
			else
				dump_16b_reg_table(init_table, sizeof(init_table)/sizeof(setting_table));
	    break;

		default:
			printf("need correct cmd!\n");
		return -1;
	} 

	return 0;
}