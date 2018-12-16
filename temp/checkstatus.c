typedef struct svc_list_s {
  char* name;
  int   svc_num;
} svc_list_t;


svc_list_t svc_list[] = { {"Sensor Manager  ", 0},
                          {"Power Manager  ", 1},
                          {"Sensor Message Router on the DSPS  ", 2},
                          {"Sensor Registry -- legacy  ", 3},
                          {"Algorithm: Absolute Motion Detection  ", 4},
                          {"Algorithm: Relative Motion Detection  ", 5},
                          {"Algorithm: Vehicle Motion Detection  ", 6},
                          {"Sensors Debug Interface on DSPS  ", 7},
                          {"Sensors Diag Interface on DSPS  ", 8},
                          {"Face and Shake service on the Apps Processor  ", 9},
                          {"Algorithm: Bring to Ear  ", 10}
};


printf("Sending ver request to %s(%d)... ", svc_list[svc_id].name, svc_list[svc_id].svc_num );

for( i = 0; i < (int)(sizeof(svc_list) / sizeof(svc_list_t)); i++ ) {
    write_ver_req( hndl_ptr, i );
}


#include <stdio.h>


typedef struct BOARD_TYPE {
  char* board_name;
  int   type_num;
} board_type;

board_type board_list[] = {
	{"gate", 1},
    {"gun", 2},
	{"desk", 3}
};

struct BOARD_ADDR {
  int  board_type_num;// 板类型号
  int  gate_num;// 柜编号
  int  board_num;// 板编号
  u8   board_addr;// 板地址
};

struct BOARD_ADDR board_addr;

int board_type(int i)
{
	for( i = 0; i < (int)(sizeof(board_list) / sizeof(board)); i++ )
		printf("Sending ver request to %s(%d)...\n", board_list[i].board_name, board_list[i].type_num );
	
	return i;
}

u8 i2c_read_board_addr(void)
{
	u8 eeprom_data = 0x12;//获取的
	
	return eeprom_data;
}

u8 get_board_addr(void)
{
	int cmd_addr = 0x12;
	
	return cmd_addr;
}

//gun/desk_num -- io_num + board_type_num + board_num Decryption and encryption
// 1 



int read_io_num(void)
{
	int in_num = 3;//获取的
	
	return in_num;
}


int decryption_board_addr(u8 addr)
{
    addr << 
	board_addr.board_type_num = board_list[i].type_num;
	board_addr.board_addr = (u8)(board_addr.board_type_num>>8 + board_addr.board_num);
	
	return addr;
}

int main (void)
{
	int ret = 0;
	
	ret = board_type();
}

