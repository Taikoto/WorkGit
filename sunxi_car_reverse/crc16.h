
#ifndef _CRC16_H_
#define _CRC16_H_

unsigned short crc16_ccitt(unsigned char *buf, int len);
unsigned char crc16_CheckSumOK(const unsigned char *buf,int len);
#endif /* _CRC16_H_ */
