CROSS_COMPILE ?=/home/taikoto/workspace/9tripod/buildroot-2017.11.2/output/host/opt/ext-toolchain/bin/arm-cortex_a9-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc

OBJS=obj/dev.o obj/merrno.o obj/print.o obj/service.o
CFLAGS=-Iinclude -lpthread

all: bin/server

bin/server: src/server.c $(OBJS)
	${CC} -o $@ $^ $(CFLAGS)

obj/service.o: src/service.c include/merrno.h
	${CC} -o $@ -c $< $(CFLAGS)

obj/dev.o: src/dev.c include/dev.h include/merrno.h
	${CC} -o $@ -c $< $(CFLAGS)

obj/merrno.o: src/merrno.c include/merrno.h
	${CC} -o $@ -c $< $(CFLAGS)

obj/print.o: src/print.c include/print.h include/huffman.h
	${CC} -o $@ -c $< $(CFLAGS)

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) bin/server

.PHONY: clean distclean all
