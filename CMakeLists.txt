cmake_minimum_required(VERSION 3.10)
project(libsombrero VERSION 1.0.0 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Basic settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED OFF)

# Enable debug symbols by default for Debug builds, O3 for Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    set(HAVE_DEBUG 1)
endif()

# Checks for programs
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)

# Checks for headers
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(unistd.h HAVE_UNISTD_H)

# Checks for functions & libraries
check_function_exists(bzero HAVE_BZERO)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(qsort HAVE_QSORT)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)

check_library_exists(m sqrtf "" HAVE_LIBM)
if(NOT HAVE_LIBM)
    message(FATAL_ERROR "Need sqrtf (libm)")
endif()

set(CMAKE_REQUIRED_LIBRARIES m)

# Find cfitsio
find_package(PkgConfig REQUIRED)
pkg_check_modules(CFITSIO REQUIRED cfitsio)
include_directories(${CFITSIO_INCLUDE_DIRS})
link_directories(${CFITSIO_LIBRARY_DIRS})

# Options for SIMD Optimization
option(ENABLE_SSE42 "Enable SSE42 optimizations" ON)
option(ENABLE_AVX "Enable AVX optimizations" ON)
option(ENABLE_AVX2 "Enable AVX2 optimizations" ON)
option(ENABLE_AVX512 "Enable AVX512 optimizations" ON)
option(ENABLE_FMA "Enable FMA optimizations" ON)

include(CheckCCompilerFlag)

if(ENABLE_SSE42)
    check_c_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
    if(COMPILER_SUPPORTS_SSE42)
        set(HAVE_SSE42 1)
        set(SSE42_CFLAGS "-DOPS_SSE42 -msse4.2 -ffast-math -ftree-vectorizer-verbose=0")
    else()
        message(FATAL_ERROR "Need a version of gcc with -msse4.2")
    endif()
endif()

if(ENABLE_AVX)
    check_c_compiler_flag("-mavx" COMPILER_SUPPORTS_AVX)
    if(COMPILER_SUPPORTS_AVX)
        set(HAVE_AVX 1)
        set(AVX_CFLAGS "-DOPS_AVX -mavx -ffast-math -ftree-vectorizer-verbose=0")
    else()
        message(FATAL_ERROR "Need a version of gcc with -mavx")
    endif()
endif()

if(ENABLE_AVX2)
    check_c_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    if(COMPILER_SUPPORTS_AVX2)
        set(HAVE_AVX2 1)
        set(AVX2_CFLAGS "-DOPS_AVX2 -mavx2 -ffast-math -ftree-vectorizer-verbose=0")
    else()
        message(FATAL_ERROR "Need a version of gcc with -mavx2")
    endif()
endif()

if(ENABLE_AVX512)
    check_c_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
    if(COMPILER_SUPPORTS_AVX512)
        set(HAVE_AVX512 1)
        set(AVX512_CFLAGS "-DOPS_AVX512 -mavx512f -ffast-math -ftree-vectorizer-verbose=0")
    else()
        message(FATAL_ERROR "Need a version of gcc with -mavx512f")
    endif()
endif()

if(ENABLE_FMA)
    check_c_compiler_flag("-mfma" COMPILER_SUPPORTS_FMA)
    if(COMPILER_SUPPORTS_FMA)
        set(HAVE_FMA 1)
        set(FMA_CFLAGS "-DOPS_FMA -mfma -ffast-math -ftree-vectorizer-verbose=0")
    else()
        message(FATAL_ERROR "Need a version of gcc with -mfma")
    endif()
endif()

# OpenMP
option(ENABLE_OPENMP "Enable OpenMP" ON)
if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_C_FOUND)
        set(HAVE_OPENMP 1)
        set(OPENMP_CFLAGS "${OpenMP_C_FLAGS}")
    else()
        message(FATAL_ERROR "Need a version of gcc with -fopenmp")
    endif()
endif()

# Variables for libsombrero.pc.in
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir ${CMAKE_INSTALL_PREFIX}/lib)
set(includedir ${CMAKE_INSTALL_PREFIX}/include)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libsombrero.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libsombrero.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libsombrero.pc
        DESTINATION lib/pkgconfig)

# Output config.h
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Add Subdirectories
add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(examples)
add_subdirectory(doc)

enable_testing()
add_subdirectory(tests)
