#ifndef MT_FS_TESTS_UTILS_H_
#define MT_FS_TESTS_UTILS_H_

#include <stdio.h>

void log_real(char const * file,
              int const line,
              char const * const function,
              char const * const format,
              ...) __attribute__ ((__format__(printf, 4, 5)));

#define LOG_ERROR(...)                          \
    do                                          \
    {                                           \
        log_real(__FILE__,                      \
                 __LINE__,                      \
                 __func__,                      \
                 __VA_ARGS__);                  \
    }                                           \
    while (0)

#define LOG_OK(...)                             \
    do                                          \
    {                                           \
        log_real(__FILE__,                      \
                 __LINE__,                      \
                 __func__,                      \
                 __VA_ARGS__);                  \
    }                                           \
    while (0)

#define LOG_DEBUG(...)                          \
    do                                          \
    {                                           \
        log_real(__FILE__,                      \
                 __LINE__,                      \
                 __func__,                      \
                 __VA_ARGS__);                  \
    }                                           \
    while (0)

#endif /* MT_FS_TESTS_UTILS_H_ */
