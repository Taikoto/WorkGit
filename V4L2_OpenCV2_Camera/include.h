#ifndef INCLUDE_H_
#define INCLUDE_H_

extern "C" {
#include <unistd.h>
#include <error.h>
#include <fcntl.h>
#include <pthread.h>
#include <termios.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include <iostream>
#include <iomanip>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;

#endif /* INCLUDE_H_ */

