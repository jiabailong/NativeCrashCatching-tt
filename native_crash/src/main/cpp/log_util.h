//
// Created by Jekton Luo on 3/6/17.
//

#ifndef LOG_UTIL_H_
#define LOG_UTIL_H_

#define DEBUGGING

#include <android/log.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#define LOGE(tag, msg, ...)                       \
    __android_log_print(ANDROID_LOG_ERROR,              \
                        tag,                            \
                        "%s: " msg,                     \
                        __func__,                       \
                        ##__VA_ARGS__);                 \


#define LOGW(tag, msg, ...)                         \
    __android_log_print(ANDROID_LOG_WARN,               \
                        tag,                            \
                        "%s: " msg,                     \
                        __func__,                       \
                        ##__VA_ARGS__);                 \


#define LOGERRNO(tag, msg, ...)                         \
    __android_log_print(ANDROID_LOG_ERROR,                  \
                        tag,                                \
                        "%s: " msg ": %s",                  \
                        __func__,                           \
                        ##__VA_ARGS__,                      \
                        strerror(errno));                   \



#define LOGI(tag, msg, ...)                      \
    __android_log_print(ANDROID_LOG_INFO,               \
                        tag,                            \
                        "%s: " msg,                     \
                        __func__,                       \
                        ##__VA_ARGS__);                 \



#ifdef DEBUGGING

#define LOGD(tag, msg, ...)                        \
    __android_log_print(ANDROID_LOG_DEBUG,              \
                        tag,                            \
                        "%s: " msg,                     \
                        __func__,                       \
                        ##__VA_ARGS__);                 \


#else

#define LOGD(...) do {          \
} while (0)

#endif



#endif



