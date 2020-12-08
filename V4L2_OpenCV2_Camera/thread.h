#ifndef THREAD_H_
#define THREAD_H_

#include "include.h"

static __inline int tty_reset(void);

static __inline int tty_set(void);

static __inline int kbhit(void);

void *thread(void *arg);

static struct termios ori_attr, cur_attr;

extern unsigned char flag;

#endif /* THREAD_H_ */

