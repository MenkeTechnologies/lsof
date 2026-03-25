cmake_minimum_required(VERSION 3.15)
project(lsof C)

set(CMAKE_C_STANDARD 11)

# Detect dialect based on platform
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LSOF_DIALECT_DIR "linux")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(LSOF_DIALECT_DIR "darwin/libproc")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(LSOF_DIALECT_DIR "freebsd")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    set(LSOF_DIALECT_DIR "n+obsd")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    set(LSOF_DIALECT_DIR "sun")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "AIX")
    set(LSOF_DIALECT_DIR "aix")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "HP-UX")
    set(LSOF_DIALECT_DIR "hpux/pstat")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

message(STATUS "Using dialect: ${LSOF_DIALECT_DIR}")

# Set dialect-specific compile definitions
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # DARWINV is Darwin kernel major version * 100 (e.g., 2500 for macOS 25.x)
    execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE DARWIN_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "^([0-9]+)" DARWIN_MAJOR "${DARWIN_RELEASE}")
    math(EXPR DARWINV "${DARWIN_MAJOR} * 100")
    add_compile_definitions(DARWINV=${DARWINV})
    message(STATUS "DARWINV=${DARWINV}")
endif()

set(LSOF_DIALECT_PATH ${PROJECT_SOURCE_DIR}/src/dialects/${LSOF_DIALECT_DIR})

# Create symlinks for dialect headers in src/ so relative includes work
# (lib/*.c uses "../machine.h", "../lsof.h" etc.)
set(LSOF_DIALECT_HEADERS machine.h dlsof.h dproto.h)
foreach(HDR ${LSOF_DIALECT_HEADERS})
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/${HDR})
        file(CREATE_LINK
            ${LSOF_DIALECT_PATH}/${HDR}
            ${PROJECT_SOURCE_DIR}/src/${HDR}
            SYMBOLIC
        )
    endif()
endforeach()

# Generate version.h
string(TIMESTAMP LSOF_CCDATE)
file(WRITE ${PROJECT_SOURCE_DIR}/src/version.h
"#define LSOF_BLDCMT \"\"\n\
#define LSOF_CC \"${CMAKE_C_COMPILER}\"\n\
#define LSOF_CCV \"${CMAKE_C_COMPILER_VERSION}\"\n\
#define LSOF_CCDATE \"${LSOF_CCDATE}\"\n\
#define LSOF_CCFLAGS \"${CMAKE_C_FLAGS}\"\n\
#define LSOF_CINFO \"\"\n\
#define LSOF_HOST \"\"\n\
#define LSOF_LDFLAGS \"\"\n\
#define LSOF_LOGNAME \"\"\n\
#define LSOF_SYSINFO \"\"\n\
#define LSOF_USER \"\"\n\
#define LSOF_VERSION \"4.85\"\n"
)

# Source files
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB DIALECT_FILES ${LSOF_DIALECT_PATH}/*.c)
file(GLOB LIB_FILES ${PROJECT_SOURCE_DIR}/src/lib/*.c)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(lsof ${SRC_FILES} ${DIALECT_FILES} ${LIB_FILES})

# ===== Test Suite =====
enable_testing()

add_executable(check_unit test/test_unit.c)
target_include_directories(check_unit PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_compile_options(check_unit PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME check_unit COMMAND check_unit)

add_executable(check_integration test/test_integration.c)
target_include_directories(check_integration PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_compile_options(check_integration PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME check_integration COMMAND check_integration)

# ===== Benchmark Suite =====
add_executable(bench_core bench/bench_core.c)
target_include_directories(bench_core PRIVATE ${PROJECT_SOURCE_DIR}/bench)
target_compile_options(bench_core PRIVATE -Wall -Wextra -Wpedantic -O2)

# 'make check' runs all tests and writes output to check_* files
set(CHECK_SCRIPT "${CMAKE_BINARY_DIR}/run_check.sh")
file(WRITE ${CHECK_SCRIPT}
"#!/bin/sh
set -e
cd \"${CMAKE_BINARY_DIR}\"

echo '=== check_unit ==='
./check_unit > check_unit.log 2>&1 && echo PASS >> check_unit.log || echo FAIL >> check_unit.log
cat check_unit.log

echo ''
echo '=== check_integration ==='
./check_integration > check_integration.log 2>&1 && echo PASS >> check_integration.log || echo FAIL >> check_integration.log
cat check_integration.log

# Exit with failure if any test failed
grep -q FAIL check_unit.log check_integration.log && exit 1 || exit 0
")

add_custom_target(check
    COMMAND sh ${CHECK_SCRIPT}
    DEPENDS check_unit check_integration
    COMMENT "Running check suite -- output in check_*.log"
)

add_custom_target(run_benchmarks
    COMMAND bench_core
    DEPENDS bench_core
    COMMENT "Running benchmarks"
)
