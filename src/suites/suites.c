
#include "suites.h"

#define SUITE(name) test_suite const test_suite_ ## name;
#include "suites.itm"
#undef SUITE

test_suite const * const test_suites[] =
{
#define SUITE(name) &test_suite_ ## name,
#include "suites.itm"
#undef SUITE
};

size_t const test_suites_count = sizeof test_suites / sizeof *test_suites;
