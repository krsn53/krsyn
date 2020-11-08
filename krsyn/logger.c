#include "logger.h"


inline int ks_log(int type, const char* message, ...){
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
