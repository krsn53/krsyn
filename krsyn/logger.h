#pragma once

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

enum ks_log_types{
    KS_LOG_INFO,
    KS_LOG_WARNING,
    KS_LOG_ERROR,

    NUM_KS_LOG_TYPES,
};


int ks_log(int type, const char* message, ...);



#define ks_info(...) ks_log(KS_LOG_INFO , __VA_ARGS__ )
#define ks_warning(...) ks_log(KS_LOG_WARNING, __VA_ARGS__ )
#define ks_error(...) ks_log(KS_LOG_ERROR,  __VA_ARGS__ )
