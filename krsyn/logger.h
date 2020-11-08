#pragma once

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

enum krsyn_log_types{
    KS_LOG_INFO,
    KS_LOG_WARNING,
    KS_LOG_ERROR,

    NUM_KS_LOG_TYPES,
};


static inline int krsyn_log(int type, const char* message, ...){
    const unsigned pre_size = sizeof ("krsyn Warning : ");
    const char* pre[NUM_KS_LOG_TYPES]= {
        "krsyn Info    : ",
        "krsyn Warning : ",
        "krsyn Error   : "
    };
    char str[pre_size + strlen(message) + 3];
    snprintf(str, sizeof(str), "%s%s\n",pre[type], message);
    va_list va;
    va_start(va, message);
    int ret =vprintf(str, va);
    va_end(va);

    return ret;
}



#define krsyn_info(...) krsyn_log(KS_LOG_INFO , __VA_ARGS__ )
#define krsyn_warning(...) krsyn_log(KS_LOG_WARNING, __VA_ARGS__ )
#define krsyn_error(...) krsyn_log(KS_LOG_ERROR,  __VA_ARGS__ )
