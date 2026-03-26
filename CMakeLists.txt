cmake_minimum_required(VERSION 3.10)
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
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${LSOF_DIALECT_PATH}/${HDR}
            ${PROJECT_SOURCE_DIR}/src/${HDR}
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
#define LSOF_VERSION \"5.1\"\n"
)

# Source files
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB DIALECT_FILES ${LSOF_DIALECT_PATH}/*.c)
file(GLOB LIB_FILES ${PROJECT_SOURCE_DIR}/src/lib/*.c)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(lsof5 ${SRC_FILES} ${DIALECT_FILES} ${LIB_FILES})
target_compile_options(lsof5 PRIVATE
    -O2
)

# --- Cyberpunk build banners ---
add_custom_target(build-banner
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh build-start)
add_dependencies(lsof5 build-banner)
add_custom_command(TARGET lsof5 POST_BUILD
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh build-end)

add_custom_target(cyberpunk-clean
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh clean-start
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh clean-end
)

add_custom_target(distclean
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh distclean-start
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        ${CMAKE_BINARY_DIR}/build
        ${CMAKE_BINARY_DIR}/CMakeCache.txt
        ${CMAKE_BINARY_DIR}/CPackConfig.cmake
        ${CMAKE_BINARY_DIR}/CPackSourceConfig.cmake
        ${CMAKE_BINARY_DIR}/CMakeFiles
        ${CMAKE_BINARY_DIR}/cmake_install.cmake
        ${CMAKE_BINARY_DIR}/install_manifest.txt
        ${CMAKE_BINARY_DIR}/Makefile
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_BINARY_DIR}/CTestTestfile.cmake
        ${CMAKE_BINARY_DIR}/Testing
        ${CMAKE_BINARY_DIR}/bench/CMakeFiles
        ${CMAKE_BINARY_DIR}/tags
        ${CMAKE_BINARY_DIR}/GPATH
        ${CMAKE_BINARY_DIR}/GRTAGS
        ${CMAKE_BINARY_DIR}/GTAGS
        ${CMAKE_BINARY_DIR}/check_unit.log
        ${CMAKE_BINARY_DIR}/check_integration.log
        ${CMAKE_BINARY_DIR}/benchmark_core.log
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh distclean-end
)

# ===== Test Suite =====
enable_testing()

add_executable(check_unit test/test_unit.c)
target_include_directories(check_unit PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_compile_options(check_unit PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME check_unit COMMAND check_unit)

add_executable(check_integration test/test_integration.c)
target_include_directories(check_integration PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_compile_options(check_integration PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(check_integration PRIVATE
    LSOF_BUILD_PATH="${CMAKE_BINARY_DIR}/lsof5"
)
add_test(NAME check_integration COMMAND check_integration)

add_executable(check_stress test/test_stress.c)
target_include_directories(check_stress PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_compile_options(check_stress PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function)
target_compile_definitions(check_stress PRIVATE
    LSOF_BUILD_PATH="${CMAKE_BINARY_DIR}/lsof5"
)
add_test(NAME check_stress COMMAND check_stress)

# 'make leakcheck' runs unit tests under macOS leaks tool
add_custom_target(leakcheck
    DEPENDS check_unit check_integration check_stress
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  Running memory leak analysis..."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND leaks --atExit -q -- $<TARGET_FILE:check_unit>
    COMMAND leaks --atExit -q -- $<TARGET_FILE:check_integration>
    COMMAND leaks --atExit -q -- $<TARGET_FILE:check_stress>
)

# ===== Benchmark Suite =====
set(BENCH_SUITES hex hash strsafe strops strbuild sort datastruct memory syscall output optduel environ)
foreach(SUITE ${BENCH_SUITES})
    add_executable(benchmark_${SUITE} bench/bench_${SUITE}.c)
    target_include_directories(benchmark_${SUITE} PRIVATE ${PROJECT_SOURCE_DIR}/bench)
    target_compile_options(benchmark_${SUITE} PRIVATE -Wall -Wextra -Wpedantic -O2)
endforeach()

# 'make check' runs all tests
add_custom_target(check
    DEPENDS check_unit check_integration check_stress
    COMMAND ${PROJECT_SOURCE_DIR}/test/run_check.sh
        $<TARGET_FILE:check_unit>
        $<TARGET_FILE:check_integration>
        $<TARGET_FILE:check_stress>
)

# 'make stress' runs only stress/regression tests
add_custom_target(stress
    DEPENDS check_stress
    COMMAND $<TARGET_FILE:check_stress>
)

# 'make bench' runs all benchmarks
add_custom_target(bench
    COMMAND ${PROJECT_SOURCE_DIR}/bench/banner.sh start
    COMMAND $<TARGET_FILE:benchmark_hex>
    COMMAND $<TARGET_FILE:benchmark_hash>
    COMMAND $<TARGET_FILE:benchmark_strsafe>
    COMMAND $<TARGET_FILE:benchmark_strops>
    COMMAND $<TARGET_FILE:benchmark_strbuild>
    COMMAND $<TARGET_FILE:benchmark_sort>
    COMMAND $<TARGET_FILE:benchmark_datastruct>
    COMMAND $<TARGET_FILE:benchmark_memory>
    COMMAND $<TARGET_FILE:benchmark_syscall>
    COMMAND $<TARGET_FILE:benchmark_output>
    COMMAND $<TARGET_FILE:benchmark_optduel>
    COMMAND $<TARGET_FILE:benchmark_environ>
    COMMAND ${PROJECT_SOURCE_DIR}/bench/banner.sh end
)
foreach(SUITE ${BENCH_SUITES})
    add_dependencies(bench benchmark_${SUITE})
endforeach()

# 'make clean' also removes the lsof binary from the source root if present
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
    "${PROJECT_SOURCE_DIR}/lsof5"
    "${PROJECT_SOURCE_DIR}/benchmark_core"
)

# ===== Linux Packaging (CPack) =====
install(CODE "execute_process(COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh install-start)")
install(TARGETS lsof5 RUNTIME DESTINATION sbin)
install(FILES ${PROJECT_SOURCE_DIR}/lsof.1 DESTINATION share/man/man1)
install(FILES ${PROJECT_SOURCE_DIR}/00MANIFEST DESTINATION share/doc/lsof OPTIONAL)
install(CODE "execute_process(COMMAND ${PROJECT_SOURCE_DIR}/scripts/banner.sh install-end)")

set(CPACK_PACKAGE_NAME "lsof")
set(CPACK_PACKAGE_VERSION "5.1")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "List open files — diagnostic tool for UNIX systems")
set(CPACK_PACKAGE_DESCRIPTION "lsof (List Open Files) maps the topology between processes and the files they hold open: regular files, directories, sockets, pipes, devices, streams — anything the kernel touches.")
set(CPACK_PACKAGE_CONTACT "lsof-maintainers")
set(CPACK_PACKAGE_VENDOR "lsof")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/00MANIFEST")

# DEB
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "Permissive")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")

set(CPACK_GENERATOR "DEB;RPM;TGZ")

include(CPack)
