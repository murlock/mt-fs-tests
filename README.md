MT-FS-TESTS
===========

MT-FS-TESTS, initially for Multi-Threaded FileSystem Tests, is a light framework for writing multi-threaded test suites.

It has been developed for internal use at Nuage Labs in order to check the reliability of one of our products
against race conditions at the filesystem level, but the framework has been designed to be more flexible than that.

By default, all suites are executed for 1 run of 500 threads, using the current working directory to create test files:

```
Usage: mt-fs-tests [<nb threads> [<nb runs> [<selected suite>]]]
```

Existing test suites
--------------------

- file_create_suite: test for race in file creation using O_CREAT|O_EXCL
- file_removal_suite: idem with file removal
- file_rename_suite: idem with file renaming
- directory_create_suite: directory creation
- directory_removal_suite: directory removal
- open_during_create_suite: test that threads either get ENOENT or 0 while trying to open a file during its creation
- bonnie64_suite: replicate some tests done by the bonnie64 tool

Writing new suites
------------------

In order to write a new suite, you need to create a new .c file in the src/suites directory,
exporting a test_suite struct named test_suite_<suite name> with the corresponding functions:
- test_suite_init: called before running the suite, so before threads are started, and returns suite data in test_suite_data
- test_suite_thread_run: this function is called by each thread, once for each run
- test_suite_post_run: this function is called after the run has been completed
- test_suite_deinit this function is called at the end, and is expected to free any resource allocated by test_suite_init

The new suite should then be added to src/suites/suites.itm, and the program rebuilt.

Compilation
-----------

MT-FS-TESTS build is based on cmake, and requires libpthread.

To build in debug mode, you can simply do:

```
mkdir build && cd build && cmake ../src && make
```

In order to do a release build:
```
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../src/ && make
```

This builds the mt-fs-tests binary into the bin/ folder.
