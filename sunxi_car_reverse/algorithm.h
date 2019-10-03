#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__


#include "base_types.h"


//COBS计算公式
#define  FinishBlock(X) (*code_ptr = (X), code_ptr = dst++, code = 0x01)


extern u16 CRCTable16(u8 *pu8Msg, u8 u8DataLen);
extern u8 StuffData(u8 *ptr, u32 length, u8 *dst);
extern u32 UnStuffData(const u8 *ptr, u8 *dst, u16 MaxLen);
extern s32 MiddleValue_AverageFilter(u16 *Data, u16 Len, u16 *Result);

u16 Upcomputer_crc16(u16 crc, const u8 *buffer, u32 len);

u8 bcd_decimal(u8 bcd);

u8 decimal_bcd(u8 decimal);

unsigned int ilog2(unsigned int v);

u8 ChangeString09ToBCD(u8 dath, u8 datl);

#endif
