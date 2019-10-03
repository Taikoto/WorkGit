#ifndef _COBS_H_
#define _COBS_H_

#define COBSADDLEN  2

void Cobs_Encrypt(const unsigned char *ptr, unsigned long length,unsigned char *dst);
void Cobs_Decrypt(const unsigned char *ptr, unsigned long length,unsigned char *dst);

#endif

