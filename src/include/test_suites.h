#ifndef TEST_SUITES_H_
#define TEST_SUITES_H_

#include <stdlib.h>

typedef enum
{
    test_suite_type_none = 0,
    test_suite_type_single,
    test_suite_type_mt,
    test_suite_type_count
} test_suite_type;

typedef int (test_suite_init)(void ** test_suite_data,
                              size_t nb_threads);

typedef int (test_suite_thread_run)(void * test_suite_data,
                                    size_t id);

typedef int (test_suite_post_run)(void * test_suite_data);

typedef int (test_suite_deinit)(void * test_suite_data);

typedef struct
{
    char const * const name;
    test_suite_init * init;
    test_suite_thread_run * run;
    test_suite_post_run * post_run;
    test_suite_deinit * deinit;
    test_suite_type type;
} test_suite;

#endif /* TEST_SUITES_H_ */
