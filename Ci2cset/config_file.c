#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CONFIG_FILE  "/home1/caihuanming/workspace/test/WorkGit/0224/regmap.txt"

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
					sscanf(reg, "%[^,]", reg);
					sscanf(line, "%*s %*s %s", val);
					sscanf(val, "%[^}]", val);					
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

int main(void)
{
	int i = 0;
	i2cconfig i2c = {0};
	setting_table *table;
	
	table = (void *)malloc(table_len);
	ctable = (void *)malloc(table_len);
	
	ci2c = get_i2cconfig_struct(i2c);
	printf("num is %d, addr is 0x%x\n",ci2c.num,ci2c.addr);
	ctable = get_setting_table(table);
	printf("get_setting_table:\n");
	for(i = 0; i < table_len; i++)
	{
		printf("{0x%x,0x%x}\n",ctable[i].cmd,ctable[i].data);
	}
	
	return 0;
}
