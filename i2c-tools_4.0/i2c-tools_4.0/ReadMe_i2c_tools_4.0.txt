I2C-Tools (4.0)
The I2C tools that used to be part of the lm-sensors package have been split to a separate package. The rationale for that move is that not all hardware monitoring chips are I2C devices, and not all I2C devices are hardware monitoring chips, so having everything in a single package was confusing and impractical.
The i2c-tools package contains a heterogeneous set of I2C tools for Linux: a bus probing tool, a chip dumper, register-level SMBus access helpers, EEPROM decoding scripts, EEPROM programming tools, and a python module for SMBus access. All versions of Linux are supported, as long as I2C support is included in the kernel.

下载地址
https://i2c.wiki.kernel.org/index.php/I2C_Tools 或者
git clone git://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git

执行文件
i2c-tools -4.0包含五个执行文件
i2cdetect – 显示所有的i2c适配器(控制器)及其使用的外设
i2cdump –读取外设所有register的值
i2cget – 读取外设某个register的值
i2cset – 向外设某个register写入值
i2ctransfer –读写i2c设备的数据 -4.0版本添加

帮助信息
i2ctransfer –help信息                                    
Usage: i2ctransfer [-f] [-y] [-v] [-V] I2CBUS DESC [DATA] [DESC [DATA]]...
  I2CBUS is an integer or an I2C bus name
  DESC describes the transfer in the form: {r|w}LENGTH[@address]
    1) read/write-flag  2) LENGTH (range 0-65535)  3) I2C address (use last one if omitted)
  DATA are LENGTH bytes for a write message. They can be shortened by a suffix:
    = (keep value constant until LENGTH)    //写入LENGTH个相同的数据
    + (increase value by 1 until LENGTH)     //写入LENTH个数据，数据递加
		- (decrease value by 1 until LENGTH)    //写入LENTH个数据，数据递减
p (use pseudo random generator until LENGTH with value as seed)   //写入随机数

Example (bus 0, read 8 byte at offset 0x64 from EEPROM at 0x50):
  # i2ctransfer 0 w1@0x50 0x64 r8
Example (same EEPROM, at offset 0x42 write 0xff 0xfe ... 0xf0):
  # i2ctransfer 0 w17@0x50 0x42 0xff-
eg：# i2ctransfer -f -y 1 w1@0x33 0x55 r5
0x67 0x76 0x30 0x30 0x32

-f 强制执行，否则会提示Device or resource busy
-y 默认执行yes ，否则会提示确认执行Continue? [Y/n] Y

i2cdetect ，i2cdump ，i2cget ，i2cset不带参数执行时会显示对应的帮助信息，可参考。
例：列举i2c总线
i2cdetect -l
列举 I2C bus i2c-1 上面连接的所有i2c设备
i2cdetect -y 1
如：读取I2C 设备(地址为0x33)寄存器0x55的值，W表示读取长度为一个word，默认为一个字节
i2cget -y -f 1 0x33 0x55 w                          
0x7667
设置 I2C 设备(地址为0x33)寄存器0x03的值为0x05
# i2cset -y -f 1 0x33 0x03 0x05
dump I2C 读取外设(地址为0x33)的所有寄存器值
i2cdump -y -f 1 0x33

参考资料：
http://blog.csdn.net/lqxandroid2012/article/details/48527887