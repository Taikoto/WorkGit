CROSS_COMPILE ?=/home/taikoto/workspace/9tripod/buildroot-2017.11.2/output/host/opt/ext-toolchain/bin/arm-cortex_a9-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc 
LD = $(CROSS_COMPILE)ld

#MYDIR := /home/wjt/linux/ebf_6ull_buildroot/output/build/ffmpeg-3.4.6/  \
	/home/wjt/app/libso/x264/
_AllDirs := $(shell ls  -R | grep '^\./.*:$$' | awk '{gsub(":","");print}')
AllDirs :=  $(_AllDirs)  .

_INCLUDE := $(AllDirs) $(MYDIR)

INCLUDE := $(patsubst %, -I %, $(_INCLUDE))
DIR_OBJ = ./obj/

SOBJ := $(foreach dir, $(AllDirs), $(wildcard $(dir)/*.c))
OBJ := $(patsubst %.c,$(DIR_OBJ)%.o,$(notdir $(SOBJ)))

TARGET = v4l2

CFLAGS = -g -Wall 
LFLAGS = -L./libyuv/ -lyuv \
         -lsupc++ \
         -lstdc++
		 #-L /home/wjt/linux/nfs/rootfs/usr/lib/ -lx264
		 #-L /home/wjt/linux/nfs/rootfs/usr/lib/ -lavcodec

VPATH = $(AllDirs)

all:$(TARGET)

$(TARGET):$(OBJ)
	$(CC)   $^  -o $@  $(LFLAGS)

$(OBJ): obj/%.o : %.c
	$(CC)  -c  $< -o $@   $(CFLAGS)  $(INCLUDE)  

.PHONY:cl 
cp:
	cp $(TARGET) /home/taikoto/workspace/nfs/  -rf
cl:
	rm $(OBJ) $(TARGET)
t:
	@echo $(INCLUDE)
