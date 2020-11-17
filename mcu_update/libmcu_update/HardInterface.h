#ifndef ANDROID_HARDWARE_MCU_HARDWAREINTERFACE_PROWAVE_H
#define ANDROID_HARDWARE_MCU_HARDWAREINTERFACE_PROWAVE_H

#include <fcntl.h>
#include <utils/threads.h>
#define INVALID_IO_TYPE -1

	 enum MCUIOTYPE { INVALIDIOTYP = -1, EUARTIO, I2CIO };

     class McuHardWareIo{
            public:
					McuHardWareIo( ):
					miotype(INVALIDIOTYP),
			        fd(INVALID_IO_TYPE)
			        {
					  fd = init_dev( miotype);
					}
							
					virtual ~McuHardWareIo()
					{
					}


                    virtual int init_dev( MCUIOTYPE iotype)
					{
					   return INVALID_IO_TYPE;
					}

                    virtual int txdata(unsigned char *txbuf, unsigned int txcount)=0;
					virtual int rxdata(unsigned char *rxbuf, unsigned int rxcount)=0;

                   MCUIOTYPE miotype;
				   int fd;

			private:

	 };


#endif
