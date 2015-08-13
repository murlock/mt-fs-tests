
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "test_suites.h"
#include "utils.h"

#define FILENAME_TEMPLATE "open_during_create_suite_XXXXXX"
#define ATTEMPTS_PER_THREAD (100)

typedef struct
{
    int results[ATTEMPTS_PER_THREAD];
} open_during_create_mt_thread_results;

typedef struct
{
    char * filename;
    open_during_create_mt_thread_results * threads_results;
    size_t nb_threads;
} open_during_create_mt_data;

static int open_during_create_mt_init(void ** test_suite_data,
                                      size_t const nb_threads)
{
    int result = 0;
    assert(test_suite_data != NULL);
    open_during_create_mt_data * data = malloc(sizeof *data);

    if (data != NULL)
    {
        data->threads_results = malloc(sizeof *(data->threads_results) * nb_threads);

        if (data->threads_results != NULL)
        {
            for (size_t idx = 0;
                 idx < nb_threads;
                 idx++)
            {
                for (size_t th_idx = 0;
                     th_idx < ATTEMPTS_PER_THREAD;
                     th_idx++)
                {
                    data->threads_results[idx].results[th_idx] = -1;
                }
            }

            data->nb_threads = nb_threads;

            data->filename = strdup(FILENAME_TEMPLATE);

            if (data->filename != NULL)
            {
                if (mktemp(data->filename) != NULL)
                {
                    *test_suite_data = data;
                }
                else
                {
                    result = errno;
                }

                if (result != 0)
                {
                    free(data->filename), data->filename = NULL;
                }
            }
            else
            {
                result = ENOMEM;
            }

            if (result != 0)
            {
                free(data->threads_results), data->threads_results = NULL;
            }
        }
        else
        {
            result = ENOMEM;
        }

        if (result != 0)
        {
            free(data), data = NULL;
        }
    }
    else
    {
        result = ENOMEM;
    }

    return result;
}

static int open_during_create_mt_run(void * test_suite_data,
                                     size_t id)
{
    int result = 0;
    open_during_create_mt_data * data = test_suite_data;
    int fd = -1;
    assert(data != NULL);
    assert(data->filename != NULL);

    for (size_t idx = 0;
         idx < ATTEMPTS_PER_THREAD;
         idx++)
    {
        bool created = false;
        assert(data->threads_results[id].results[idx] == -1);

        result = 0;

        if (id == (data->nb_threads / 2) &&
            (ATTEMPTS_PER_THREAD > 1 &&
             idx == ATTEMPTS_PER_THREAD / 2))
        {
            fd = open(data->filename,
                      O_CREAT | O_EXCL | O_WRONLY,
                      S_IRUSR | S_IWUSR);
            created = true;
        }
        else
        {
            fd = open(data->filename,
                      O_RDONLY);
        }

        if (fd != -1)
        {
            if (created == true)
            {
                static char buff[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCDDDDDDDDDDDDDDDDDDEEEEEEEEEEEEEFFFFFFFFFFFFFFFFFf";
                ssize_t const written = write(fd,
                                              buff,
                                              sizeof buff);

                if (written != sizeof buff)
                {
                    result = errno;
                    LOG_ERROR("Error writing file content (%zu / %zu): %d",
                              written,
                              sizeof buff,
                              result);
                }
            }

            data->threads_results[id].results[idx] = result;
            close(fd), fd = -1;
        }
        else
        {
            data->threads_results[id].results[idx] = errno;
        }
    }

    return result;
}

static int open_during_create_mt_post_run(void * test_suite_data)
{
    int result = 0;
    open_during_create_mt_data * data = test_suite_data;
    size_t enoent_count = 0;
    size_t ok_count = 0;
    size_t invalid_count = 0;
    assert(data != NULL);

    for (size_t th_idx = 0;
         th_idx < data->nb_threads;
         th_idx++)
    {
        for (size_t idx = 0;
             idx < ATTEMPTS_PER_THREAD;
             idx++)
        {
            if (data->threads_results[th_idx].results[idx] == 0)
            {
                ok_count++;
            }
            else if (data->threads_results[th_idx].results[idx] == ENOENT)
            {
                enoent_count++;
            }
            else
            {
                LOG_ERROR("Thread %zu got %d",
                          th_idx,
                          data->threads_results[th_idx].results[idx]);
                invalid_count++;
            }
        }
    }

    if ((enoent_count + ok_count) == (data->nb_threads * ATTEMPTS_PER_THREAD) &&
        ok_count >= 1)
    {
        LOG_OK("Success (%zu / %zu / %zu)!",
               ok_count,
               enoent_count,
               invalid_count);
    }
    else
    {
        LOG_ERROR("Error, we got %zu valid open, %zu does not exist and %zu invalid return codes.",
                  ok_count,
                  enoent_count,
                  invalid_count);
    }

    return result;
}

static int open_during_create_mt_deinit(void * test_suite_data)
{
    int result = 0;
    open_during_create_mt_data * data = test_suite_data;

    if (data != NULL)
    {
        if (data->threads_results != NULL)
        {
            free(data->threads_results), data->threads_results = NULL;
        }

        if (data->filename != NULL)
        {
            unlink(data->filename);
            free(data->filename), data->filename = NULL;
        }

        free(data);
    }

    return result;
}

test_suite const test_suite_open_during_create_mt =
{
    "open_during_create_mt",
    &open_during_create_mt_init,
    &open_during_create_mt_run,
    &open_during_create_mt_post_run,
    &open_during_create_mt_deinit,
    test_suite_type_mt
};
