
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

void log_real(char const * file,
              int const line,
              char const * const function,
              char const * const format,
              ...)
{
    assert(file != NULL);
    assert(function != NULL);
    assert(format != NULL);
    size_t const file_len = strlen(file);

    if (file_len > 0)
    {
        size_t file_pos = file_len - 1;

        while(file_pos > 0 && file[file_pos] != '/')
        {
            file_pos--;
        }

        if (file_pos > 0)
        {
            file += file_pos + 1;
        }
    }

    fprintf(stderr,
            "[%s(%d) %s] ",
            file, line, function);

    va_list params;
    va_start(params, format);
    vfprintf(stderr, format, params);
    va_end(params);

    fputs("\n", stderr);
    fflush(stderr);
}
