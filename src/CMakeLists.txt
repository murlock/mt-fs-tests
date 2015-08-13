cmake_minimum_required(VERSION 2.8)

project(MTFSTests C)

if (NOT DEFINED ENV{COMPILER})
   #set (CMAKE_C_COMPILER "/usr/lib/clang-analyzer/scan-build/ccc-analyzer")
   set (CMAKE_C_COMPILER "gcc")
   #set (CMAKE_C_COMPILER "clang")
else()
   set (CMAKE_C_COMPILER "$ENV{COMPILER}")
endif()

option(ENABLE_COVERAGE "Enable Code Coverage" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)

set(CMAKE_C_FLAGS "$ENV{CFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "$ENV{LDFLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "$ENV{LDFLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "$ENV{LDFLAGS}")

LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules/")

if(CMAKE_COMPILER_IS_GNUCC OR ${CMAKE_C_COMPILER} STREQUAL "clang")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wstrict-aliasing -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Werror-implicit-function-declaration -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wconversion -pipe -Wextra -Wcast-align -Wno-missing-field-initializers -Wmain -Waddress -fstack-protector -fPIC -fvisibility=hidden")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro,-z,now")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro,-z,now")

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        message("Release build")

        set(CMAKE_C_FLAGS_RELEASE "-D_FORTIFY_SOURCE=2 -O3 -Wuninitialized -DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")
        set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "-s")

        if (${CMAKE_C_COMPILER} STREQUAL "gcc")
            message("Compiler is GCC")

            if(DEFINED CMAKE_C_COMPILER_VERSION AND ("${CMAKE_C_COMPILER_VERSION}" VERSION_EQUAL "4.6.0" OR "${CMAKE_C_COMPILER_VERSION}" VERSION_GREATER "4.6.0"))
                message("Enabling LTO")
                set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fuse-linker-plugin -flto")
                set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wsuggest-attribute=const -Wsuggest-attribute=pure")
            endif()

        elseif(${CMAKE_C_COMPILER} STREQUAL "clang")
            message("Compiler is Clang")
            message("Enabling LTO")
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
        else()
            message("Compiler is other : ${CMAKE_C_COMPILER}")
        endif()

    else()
        message("Debug build.")
        set(CMAKE_BUILD_TYPE "Debug")
        set(CMAKE_C_FLAGS_DEBUG "-Wunreachable-code -O0 -fno-omit-frame-pointer ")

        if (${CMAKE_C_COMPILER} STREQUAL "gcc")
            message("Compiler is GCC")

            set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3")

            if(DEFINED CMAKE_C_COMPILER_VERSION)
                message("GCC version is ${CMAKE_C_COMPILER_VERSION}")
                if("${CMAKE_C_COMPILER_VERSION}" VERSION_EQUAL "4.8.0" OR "${CMAKE_C_COMPILER_VERSION}" VERSION_GREATER "4.8.0")
                    if (ENABLE_ASAN)
                        message("Address Sanitizer is ON")
                        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fsanitize=address")
                    endif(ENABLE_ASAN)
                endif()
            endif()

            if (ENABLE_COVERAGE)
                message("Coverage is on.")
                set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} --coverage")
            endif()
        elseif(${CMAKE_C_COMPILER} STREQUAL "clang")
            message("Compiler is Clang")
             if (ENABLE_ASAN)
                 message("Address Sanitizer is ON")
                 set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -fsanitize=address")
             endif(ENABLE_ASAN)
        else()
            message("Compiler is other : ${CMAKE_C_COMPILER}")
            set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3")
        endif()

    endif()

endif()

include_directories(include/)
include_directories(.)

set(EXECUTABLE_OUTPUT_PATH bin)
set(LIBRARY_OUTPUT_PATH lib)

add_executable(mt-fs-tests
               mt-fs-tests.c
               utils.c
               suites/bonnie64_suite.c
               suites/directory_create_suite.c
               suites/directory_removal_suite.c
               suites/file_create_suite.c
               suites/file_removal_suite.c
               suites/file_rename_suite.c
               suites/open_during_create_suite.c
               suites/suites.c
               )

target_link_libraries(mt-fs-tests pthread)

install(TARGETS mt-fs-tests
        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
