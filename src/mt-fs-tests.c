
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

#define DEFAULT_THREADS_COUNT (500)

#include "suites/suites.h"
#include "barrier.h"
#include "utils.h"

typedef struct
{
    mt_fs_tests_barrier_t barrier;
    test_suite const * selected_suite;
    size_t nb_runs;
    size_t nb_threads;
} global_params;

typedef struct
{
    pthread_t thread;
    global_params * params;
    test_suite const * suite;
    void * suite_data;
    size_t id;
} thread_params;

static void * suite_thread_run(void * th_data)
{
    int result = 0;
    thread_params * th_params = th_data;
    assert(th_params != NULL);
    assert(th_params->params != NULL);
    assert(th_params->suite != NULL);
    assert(th_params->suite->run != NULL);

    mt_fs_tests_barrier_wait(&(th_params->params->barrier));

    result = (*(th_params->suite->run))(th_params->suite_data,
                                        th_params->id);

    if (result != 0)
    {
        LOG_ERROR("Error in suite run for thread %zu of suite %s: %d",
                  th_params->id,
                  th_params->suite->name,
                  result);
    }

    return NULL;
}

static int run_suite(global_params * const params,
                     test_suite const * const suite)
{
    int result = 0;
    void * suite_data = NULL;

    assert(params != NULL);
    assert(suite != NULL);
    assert(suite->name != NULL);
    assert(suite->run != NULL);
    assert(suite->type > test_suite_type_none);
    assert(suite->type < test_suite_type_count);

    if (suite->init != NULL)
    {
        result = (*(suite->init))(&suite_data,
                                  params->nb_threads);

        if (result != 0)
        {
            LOG_ERROR("Error in suite initialization for suite %s: %d",
                      suite->name,
                      result);
        }
    }

    if (result == 0)
    {
        if (suite->type == test_suite_type_single)
        {
            result = (*(suite->run))(suite_data,
                                     0);

            if (result != 0)
            {
                LOG_ERROR("Error running suite %s: %d",
                          suite->name,
                          result);
            }
        }
        else if (suite->type == test_suite_type_mt)
        {
            thread_params * threads_params = malloc(sizeof *threads_params * params->nb_threads);

            if (threads_params != NULL)
            {
                size_t idx = 0;
                assert(params->nb_threads <= UINT_MAX);

                result = mt_fs_tests_barrier_init(&(params->barrier),
                                                  (unsigned int) params->nb_threads);

                if (result == 0)
                {
                    for (idx = 0;
                         result == 0 &&
                             idx < params->nb_threads;
                         idx++)
                    {
                        thread_params * tp = &(threads_params[idx]);
                        tp->id = idx;
                        tp->suite_data = suite_data;
                        tp->params = params;
                        tp->suite = suite;

                        result = pthread_create(&(tp->thread),
                                                NULL,
                                                &suite_thread_run,
                                                tp);

                        if (result != 0)
                        {
                            LOG_ERROR("Creation of thread %zu for suite %s failed: %d\n",
                                      idx,
                                      suite->name,
                                      result);
                        }
                    }

                    if (result != 0)
                    {
                        for (size_t cancel_idx = 0;
                             cancel_idx < idx;
                             cancel_idx++)
                        {
                            thread_params * tp = &(threads_params[cancel_idx]);
                            pthread_cancel(tp->thread);
                        }
                    }

                    for(size_t join_idx = 0;
                        join_idx < idx;
                        join_idx++)
                    {
                        thread_params * tp = &(threads_params[join_idx]);

                        result = pthread_join(tp->thread,
                                              NULL);

                        if (result != 0)
                        {
                            LOG_ERROR("Error joining thread %zu for suite %s: %d",
                                      join_idx,
                                      suite->name,
                                      result);
                        }
                    }

                    mt_fs_tests_barrier_destroy(&(params->barrier));
                }
                else
                {
                    LOG_ERROR("Error creating barrier for suite %s: %d",
                              suite->name,
                              result);
                }

                free(threads_params), threads_params = NULL;
            }
            else
            {
                result = ENOMEM;
                LOG_ERROR("Error allocating threads data for suite %s: %d",
                          suite->name,
                          result);
            }
        }
        else
        {
            LOG_ERROR("Invalid suite type %d for suite %s",
                      suite->type,
                      suite->name);
            result = EINVAL;
        }
    }

    if (result == 0 &&
        suite->post_run != NULL)
    {
        result = (*(suite->post_run))(suite_data);

        if (result != 0)
        {
            LOG_ERROR("Error in post run action for suite %s: %d",
                      suite->name,
                      result);
        }
    }

    if (suite->deinit != NULL)
    {
        result = (*(suite->deinit))(suite_data);

        if (result != 0)
        {
            LOG_ERROR("Error in suite deinitialization for suite %s: %d",
                      suite->name,
                      result);
        }
    }

    return result;
}

static int str_to_unsigned_int64(char const * const str_val,
                                 uint64_t * const out)
{
    int result = 0;

    if (str_val != NULL && out != NULL)
    {
        long long int val = 0;
        char *endptr;

        errno = 0;
        val = strtoll(str_val, &endptr, 10);

        /* Check for various possible errors */
        if ((errno == ERANGE && (val == LLONG_MAX || val == LLONG_MIN))
            || (errno != 0 && val == 0))
        {
            result = ERANGE;
        }
        else if (endptr == str_val)
        {
            result = ENOENT;
        }
        else if (val < 0 || (unsigned long long) val > UINT64_MAX)
        {
            result = ENOENT;
        }
        else
        {
            *out = (unsigned long long) val;
        }
    }
    else
    {
        result = EINVAL;
    }

    return result;
}

static int parse_params(int const argc,
                        char const * const * const argv,
                        global_params * const params)
{
    int result = 0;

    if (argc >= 2 &&
        argc <= 4)
    {
        /* nb threads */
        uint64_t temp = 0;

        result = str_to_unsigned_int64(argv[1],
                                       &temp);

        if (result == 0)
        {
            params->nb_threads = temp;

            if (argc >= 3)
            {
                /* nb runs */

                result = str_to_unsigned_int64(argv[2],
                                               &temp);

                if (result == 0)
                {
                    params->nb_runs = temp;

                    if (argc == 4)
                    {
                        /* suite name */
                        if (strcasecmp(argv[3], "all") != 0)
                        {
                            result = ENOENT;

                            for (size_t suite_idx = 0;
                                 result == ENOENT &&
                                     suite_idx < test_suites_count;
                                 suite_idx++)
                            {
                                assert(test_suites[suite_idx]->name != NULL);

                                if (strcasecmp(argv[3],
                                               test_suites[suite_idx]->name) == 0)
                                {
                                    params->selected_suite = test_suites[suite_idx];
                                    result = 0;
                                }
                            }

                            if (result != 0)
                            {
                                LOG_ERROR("Suite not found!");
                            }
                        }
                    }
                }
                else
                {
                    LOG_ERROR("Invalid number of runs!");
                }
            }
        }
        else
        {
            LOG_ERROR("Invalid number of threads!");
        }
    }
    else if (argc != 1)
    {
        result = EINVAL;
        LOG_ERROR("Usage: %s [<nb threads> [<nb runs> [<selected suite>]]]",
                  argv[0]);
    }

    return result;
}

int main(int const argc,
         char const * const * const argv)
{
    global_params params =
        {
            .nb_runs = 1,
            .nb_threads = DEFAULT_THREADS_COUNT
        };
    int result = parse_params(argc,
                              argv,
                              &params);

    if (result == 0)
    {
        LOG_OK("Launching %s with %zu runs of %zu threads",
               params.selected_suite != NULL ? params.selected_suite->name : "all suites",
               params.nb_runs,
               params.nb_threads);

        for (size_t run_idx = 0;
             result == 0 &&
                 run_idx < params.nb_runs;
             run_idx++)
        {
            if (params.selected_suite != NULL)
            {
                result = run_suite(&params,
                                   params.selected_suite);
            }
            else
            {
                for (size_t suite_idx = 0;
                     result == 0 &&
                         suite_idx < test_suites_count;
                     suite_idx++)
                {
                    result = run_suite(&params,
                                       test_suites[suite_idx]);
                }
            }
        }
    }

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    return result;
}
