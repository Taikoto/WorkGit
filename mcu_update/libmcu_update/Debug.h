#ifndef LOG_DEBUG
#define LOG_DEBUG

#include <stdio.h>
#include <cutils/log.h>
#include <android/log.h>
#include <string.h>

#define M_LOG_TAG    "Mcu_Server"
#define MLOGE(...)   __android_log_print(ANDROID_LOG_ERROR,M_LOG_TAG,__VA_ARGS__)

#define MCU_DEBUG
#ifdef MCU_DEBUG
#define MLOGI(...)   __android_log_print(ANDROID_LOG_INFO,M_LOG_TAG,__VA_ARGS__)
#define MLOGD(...)   __android_log_print(ANDROID_LOG_DEBUG,M_LOG_TAG,__VA_ARGS__)
#define MLOGW(...)   __android_log_print(ANDROID_LOG_WARN,M_LOG_TAG,__VA_ARGS__)
#else
#define MLOGI(...)
#define MLOGD(...)
#define MLOGW(...)
#endif

// namespace android
// {
    /* 打印16进制码流 */
    static void printhexType(uint8_t *src, int len, const char* szType)
    {
        if (src == NULL)
        {
            return;
        }

        if (len > (1024))
        {
            return;
        }

        char szbuff[1024 + 1] = {0};

        int i = 0;
        for (i = 0; i < len; i++)
        {
            char tmp[10] = {0};
            snprintf(tmp, 8, " %02X", src[i]);
            strcat(szbuff, tmp);
        }

    #ifdef LOG_DEBUG
        printf("%s[%d]:%s\n", szType, len, szbuff);
    #endif

        MLOGD("%s[%d]:%s", szType, len, szbuff);

        return;
    }

    static void printMcu2App(uint8_t *src, int len)
    {
        printhexType(src, len, "mcu2app");
    }

    static void printApp2Mcu(uint8_t *src, int len)
    {
        printhexType(src, len, "app2mcu");
    }

    /* 打印16进制码流 */
    static void printSockethex(uint8_t *src, int len)
    {
        printhexType(src, len, "sock2mcu");
    }


// }

#endif
