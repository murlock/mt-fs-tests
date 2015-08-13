
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "test_suites.h"
#include "utils.h"

#define FILENAME_TEMPLATE "bonnie64_suite_XXXXXX"
#define FILE_SIZE ((size_t) 8 * 1024 * 1024) /* 8 MB */
#define BUFFER_SIZE (16384)
#define ITERATIONS (4000)
#define UPDATE_EVERY_N_SEEKS (10)

typedef struct {
    char * filename;
    int * results;
    size_t nb_threads;
} bonnie64_mt_data;

static int bonnie64_mt_write_one_byte_at_a_time(char const * const filename)
{
    assert(filename != NULL);
    int result = 0;
    int fd = open(filename, O_RDWR);

    if (fd != -1)
    {
        FILE * fp = fdopen(fd, "r+");

        if (fp != NULL)
        {
            for (size_t idx = 0;
                 idx < FILE_SIZE &&
                     result == 0;
                 idx++)
            {
                putc('A', fp);

                if (ferror(fp))
                {
                    result = EIO;
                    LOG_ERROR("Error writing to file: %d",
                              errno);
                }
            }

            if (result == 0)
            {
                fflush(fp);
                fsync(fd);
            }

            fclose(fp), fp = NULL;
            fd = -1;
        }
        else
        {
            result = errno;
        }

        if (fd != -1)
        {
            close(fd), fd = -1;
        }
    }
    else
    {
        result = errno;
        LOG_ERROR("Error opening %s: %d",
                  filename,
                  result);
    }

    return result;
}

static int bonnie64_mt_rewrite_chunks(char const * const filename)
{
    assert(filename != NULL);
    int result = 0;
    int fd = open(filename, O_RDWR);

    if (fd != -1)
    {
        off_t pos = lseek(fd, 0, SEEK_SET);

        if (pos != -1)
        {
            ssize_t got = 0;

            result = 0;

            do
            {
                static char buffer[BUFFER_SIZE];
                static size_t const buffer_size = sizeof buffer;

                got = read(fd,
                           buffer,
                           buffer_size);

                if (got > 0)
                {
                    buffer[got -1] ^= 'J';

                    pos = lseek(fd, -got, SEEK_CUR);

                    if (pos != -1)
                    {
                        ssize_t const written = write(fd,
                                                      buffer,
                                                      (size_t) got);

                        if (written != got)
                        {
                            result = errno;
                            LOG_ERROR("Error writing %zu to file: %d",
                                      got,
                                      result);
                        }
                    }
                    else
                    {
                        result = errno;
                        LOG_ERROR("Error seeking in file: %d",
                                  result);
                    }
                }
                else if (got < 0)
                {
                    result = errno;
                    LOG_ERROR("Error reading from file: %d",
                              result);
                }
            }
            while (result == 0 &&
                   got > 0);

            if (result == 0)
            {
                fsync(fd);
            }
        }
        else
        {
            result = errno;
            LOG_ERROR("Error seeking in file: %d",
                      result);
        }

        if (fd != -1)
        {
            close(fd), fd = -1;
        }
    }
    else
    {
        result = errno;
        LOG_ERROR("Error opening %s: %d",
                  filename,
                  result);
    }

    return result;
}

static int bonnie64_mt_write_chunks(char const * const filename)
{
    assert(filename != NULL);
    int result = 0;
    int fd = open(filename, O_RDWR);

    if (fd != -1)
    {
        off_t pos = lseek(fd, 0, SEEK_SET);

        if (pos != -1)
        {
            static char buffer[BUFFER_SIZE];
            static size_t const buffer_size = sizeof buffer;
            size_t to_write = FILE_SIZE;

            result = 0;

            memset(buffer, 'X', buffer_size);

            do
            {
                ssize_t const written = write(fd,
                                              buffer,
                                              buffer_size > to_write ? to_write : buffer_size);

                if (written > 0)
                {
                    to_write -= (size_t) written;
                }
                else
                {
                    result = errno;
                    LOG_ERROR("Error writing %zu to file: %d",
                              buffer_size > to_write ? to_write : buffer_size,
                              result);
                }
            }
            while (result == 0 &&
                   to_write > 0);

            if (result == 0)
            {
                fsync(fd);
            }
        }
        else
        {
            result = errno;
            LOG_ERROR("Error seeking in file: %d",
                      result);
        }

        if (fd != -1)
        {
            close(fd), fd = -1;
        }
    }
    else
    {
        result = errno;
        LOG_ERROR("Error opening %s: %d",
                  filename,
                  result);
    }

    return result;
}

static int bonnie64_mt_read_one_byte_at_a_time(char const * const filename)
{
    assert(filename != NULL);
    int result = 0;
    int fd = open(filename, O_RDWR);

    if (fd != -1)
    {
        FILE * fp = fdopen(fd, "r+");

        if (fp != NULL)
        {
            for (size_t idx = 0;
                 idx < FILE_SIZE &&
                     result == 0;
                 idx++)
            {
                int const value = getc(fp);

                if (!ferror(fp))
                {
                    if (value != 'X')
                    {
                        LOG_ERROR("Error, expected %c, got %d",
                                  'X',
                                  value);
                        result = EIO;
                    }
                }
                else
                {
                    result = EIO;
                    LOG_ERROR("Error writing to file: %d",
                              errno);
                }
            }

            fclose(fp), fp = NULL;
            fd = -1;
        }
        else
        {
            result = errno;
        }

        if (fd != -1)
        {
            close(fd), fd = -1;
        }
    }
    else
    {
        result = errno;
        LOG_ERROR("Error opening %s: %d",
                  filename,
                  result);
    }

    return result;
}

static int bonnie64_mt_read_chunks(char const * const filename)
{
    assert(filename != NULL);
    int result = 0;
    int fd = open(filename, O_RDWR);

    if (fd != -1)
    {
        off_t pos = lseek(fd, 0, SEEK_SET);

        if (pos != -1)
        {
            static char buffer[BUFFER_SIZE];
            static size_t buffer_size = sizeof buffer;
            ssize_t got = 0;

            result = 0;

            do
            {
                got = read(fd,
                           buffer,
                           buffer_size);

                if (got < 0)
                {
                    result = errno;
                    LOG_ERROR("Error writing %zu to file: %d",
                              got,
                              result);
                }
            }
            while (result == 0 &&
                   got > 0);
        }
        else
        {
            result = errno;
            LOG_ERROR("Error seeking in file: %d",
                      result);
        }

        if (fd != -1)
        {
            close(fd), fd = -1;
        }
    }
    else
    {
        result = errno;
        LOG_ERROR("Error opening %s: %d",
                  filename,
                  result);
    }

    return result;
}

static int bonnie64_mt_init(void ** test_suite_data,
                            size_t const nb_threads)
{
    int result = 0;
    assert(test_suite_data != NULL);
    bonnie64_mt_data * data = malloc(sizeof *data);

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

            data->filename = strdup(FILENAME_TEMPLATE);

            if (data->filename != NULL)
            {
                int fd = mkstemp(data->filename);

                if (fd != -1)
                {
                    LOG_DEBUG("Writing one byte at a time..");
                    result = bonnie64_mt_write_one_byte_at_a_time(data->filename);

                    if (result == 0)
                    {
                        LOG_DEBUG("Reading and rewriting chunks..");
                        result = bonnie64_mt_rewrite_chunks(data->filename);

                        if (result == 0)
                        {
                            LOG_DEBUG("Writing over chunks..");
                            result = bonnie64_mt_write_chunks(data->filename);

                            if (result == 0)
                            {
                                LOG_DEBUG("Writing a byte at a time..");
                                result = bonnie64_mt_read_one_byte_at_a_time(data->filename);

                                if (result == 0)
                                {
                                    LOG_DEBUG("Reading chunks..");
                                    result = bonnie64_mt_read_chunks(data->filename);
                                    LOG_DEBUG("Init done.");
                                }
                            }
                        }
                    }

                    close(fd), fd = -1;
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
                free(data->results), data->results = NULL;
            }
        }
        else
        {
            result = ENOMEM;
        }

        if (result == 0)
        {
            *test_suite_data = data;
        }
        else
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

static int bonnie64_mt_run(void * const test_suite_data,
                              size_t const id)
{
    int result = 0;
    bonnie64_mt_data * data = test_suite_data;
    int fd = -1;

    assert(data != NULL);
    assert(data->filename != NULL);
    assert(data->results[id] == -1);

    fd = open(data->filename,
              O_RDWR);

    if (fd != -1)
    {
        static char buffer[BUFFER_SIZE];
        static size_t buffer_size = sizeof buffer;
        size_t const nb_chunks = FILE_SIZE / buffer_size;

        srandom((unsigned int) id);

        for (size_t idx = 0;
             result == 0 &&
                 idx < ITERATIONS;
             idx++)
        {
            off_t const pos = (off_t) (random() % (unsigned int) nb_chunks);

            off_t res = lseek(fd,
                              pos,
                              SEEK_SET);

            if (res != -1)
            {
                ssize_t got = read(fd,
                                   buffer,
                                   buffer_size);

                if (got > 0)
                {
                    if (idx % UPDATE_EVERY_N_SEEKS == 0)
                    {
                        buffer[got - 1] ^= 'Z';

                        res = lseek(fd,
                                    pos,
                                    SEEK_SET);

                        if (res != -1)
                        {
                            ssize_t const written = write(fd,
                                                          buffer,
                                                          buffer_size);

                            if (written > 0)
                            {
                                result = 0;
                            }
                            else
                            {
                                result = errno;
                                LOG_ERROR("Error writing to file: %d",
                                          result);
                            }
                        }
                        else
                        {
                            result = errno;
                            LOG_ERROR("Error seeking to %lld: %d",
                                      (long long int) pos,
                                      result);
                        }
                    }
                }
                else
                {
                    result = errno;
                    LOG_ERROR("Error reading from %lld: %d",
                              (long long int) pos,
                              result);
                }
            }
            else
            {
                result = errno;
                LOG_ERROR("Error seeking to %lld: %d",
                          (long long int) pos,
                          result);
            }
        }

        data->results[id] = result;
        close(fd), fd = -1;
    }
    else
    {
        result = errno;
        LOG_ERROR("Error opening file %s: %d",
                  data->filename,
                  result);
        data->results[id] = errno;
    }

    return 0;
}

static int bonnie64_mt_post_run(void * test_suite_data)
{
    int result = 0;
    bonnie64_mt_data * data = test_suite_data;
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
        else
        {
            invalid_count++;
        }
    }

    if (ok_count == data->nb_threads)
    {
        LOG_OK("Success!");
    }
    else
    {
        LOG_ERROR("Error, we got %zu success and %zu invalid return codes.",
                  ok_count,
                  invalid_count);
    }

    return result;
}

static int bonnie64_mt_deinit(void * test_suite_data)
{
    int result = 0;
    bonnie64_mt_data * data = test_suite_data;

    if (data != NULL)
    {
        if (data->results != NULL)
        {
            free(data->results), data->results = NULL;
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

test_suite const test_suite_bonnie64_mt =
{
    "bonnie64_mt",
    &bonnie64_mt_init,
    &bonnie64_mt_run,
    &bonnie64_mt_post_run,
    &bonnie64_mt_deinit,
    test_suite_type_mt
};
