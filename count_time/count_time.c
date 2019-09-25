#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <time.h>
 
void do_func(){
    int k;
    for(int i=0;i<1000;i++)
        for(int j=0;j<1000;j++)
            k=i+j;
}
//int gettimeofday(struct timeval *tv,struct timezone *tz);精度到微秒
long us_timer(){
    struct timeval start,end;
    gettimeofday(&start,NULL);
    do_func();
    gettimeofday(&end,NULL);
    return (end.tv_sec-start.tv_sec)*10^6+end.tv_usec-start.tv_usec;
}
/*
int clock_gettime(clockid_t clk_id,struct timespec *tp);精确到纳秒
CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变,即从UTC1970-1-1 0:0:0开始计时,中间时刻如果系统时间被用户改成其他,则对应的时间相应改变
CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
*/

long ns_timer(){
    struct timespec start,end;
    clock_gettime(CLOCK_MONOTONIC,&start);
    do_func();
    clock_gettime(CLOCK_MONOTONIC,&end);
    return (end.tv_sec-start.tv_sec)*10^9+end.tv_nsec-start.tv_nsec;
}
 
int main(){
    printf("花费时间 : %ld 微秒(us)\n", us_timer());
    printf("花费时间 : %ld 纳秒(ns)\n", ns_timer());
    return 0;
}
