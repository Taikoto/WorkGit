#include "cobs.h" 

#define FinishBlock(X) (*code_ptr = (X), code_ptr = dst++, code = 0x01 )

/*************************************
Name:stuffData
Parameter: 
Function:COBS数据加密
*************************************/
void Cobs_Encrypt(const unsigned char *ptr, unsigned long length,unsigned char *dst)
{
	const unsigned char *end = ptr + length;
	unsigned char *code_ptr = dst++;
	unsigned char code = 0x01;
	while (ptr < end)
	{
		if (*ptr == 0)
			FinishBlock(code);
		else
		{
			*dst++ = *ptr;
			code++;
			if (code == 0xFF)
				FinishBlock(code);
		}
		ptr++;
	}
	FinishBlock(code);
	FinishBlock(0);
}


/*************************************
Name:UnstuffData
Parameter: 
Function:COBS数据解密
*************************************/
void Cobs_Decrypt(const unsigned char *ptr, unsigned long length,unsigned char *dst)
{
	const unsigned char *end = ptr + length;
	while (ptr < end)
	{
		int i, code = *ptr++;
		for (i = 1; i < code; i++)
			*dst++ = *ptr++;
		if (code < 0xFF)
			*dst++ = 0;
	}
}
