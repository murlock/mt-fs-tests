
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "test_suites.h"
#include "utils.h"

#define DIRECTORY_NAME_TEMPLATE "directory_removal_suite_XXXXXX"

typedef struct {
    char * directory_name;
    int * results;
    size_t nb_threads;
} directory_removal_mt_data;

static int directory_removal_mt_init(void ** test_suite_data,
                                     size_t const nb_threads)
{
    int result = 0;
    assert(test_suite_data != NULL);
    directory_removal_mt_data * data = malloc(sizeof *data);

    if (data != NULL)
    {
        data->results = malloc(sizeof *(data->results) * nb_threads);

        if (data->results != NULL)
        {
            for (size_t idx = 0;
                 idx < nb_threads;
                 idx++)
            {
                data->results[idx] = -1;
            }

            data->nb_threads = nb_threads;

            data->directory_name = strdup(DIRECTORY_NAME_TEMPLATE);

            if (data->directory_name != NULL)
            {
                if (mkdtemp(data->directory_name) != NULL)
                {
                    *test_suite_data = data;
                }
                else
                {
                    result = errno;
                }

                if (result != 0)
                {
                    free(data->directory_name), data->directory_name = NULL;
                }
            }
            else
            {
                result = ENOMEM;
            }

            if (result != 0)
            {
                free(data->results), data->results = NULL;
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

static int directory_removal_mt_run(void * test_suite_data,
                                    size_t id)
{
    int result = 0;
    directory_removal_mt_data * data = test_suite_data;

    assert(data != NULL);
    assert(data->directory_name != NULL);
    assert(data->results[id] == -1);

    result = rmdir(data->directory_name);

    if (result == 0)
    {
        data->results[id] = 0;
    }
    else
    {
        data->results[id] = errno;
        result = 0;
    }

    return result;
}

static int directory_removal_mt_post_run(void * test_suite_data)
{
    int result = 0;
    directory_removal_mt_data * data = test_suite_data;
    size_t enoent_count = 0;
    size_t ok_count = 0;
    size_t invalid_count = 0;
    assert(data != NULL);

    for (size_t idx = 0;
         idx < data->nb_threads;
         idx++)
    {
        if (data->results[idx] == 0)
        {
            ok_count++;
        }
        else if (data->results[idx] == ENOENT)
        {
            enoent_count++;
        }
        else
        {
            invalid_count++;
        }
    }

    if (ok_count == 1 &&
        enoent_count == (data->nb_threads - 1))
    {
        LOG_OK("Success!");
    }
    else
    {
        LOG_ERROR("Error, we got %zu removal, %zu does not exist and %zu invalid return codes.",
                  ok_count,
                  enoent_count,
                  invalid_count);
    }

    return result;
}

static int directory_removal_mt_deinit(void * test_suite_data)
{
    int result = 0;
    directory_removal_mt_data * data = test_suite_data;

    if (data != NULL)
    {
        if (data->results != NULL)
        {
            free(data->results), data->results = NULL;
        }

        if (data->directory_name != NULL)
        {
            rmdir(data->directory_name);
            free(data->directory_name), data->directory_name = NULL;
        }

        free(data);
    }

    return result;
}

test_suite const test_suite_directory_removal_mt =
{
    "directory_removal_mt",
    &directory_removal_mt_init,
    &directory_removal_mt_run,
    &directory_removal_mt_post_run,
    &directory_removal_mt_deinit,
    test_suite_type_mt
};
