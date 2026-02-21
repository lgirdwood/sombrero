add_library(ops OBJECT
    data_ops.c
    convolution_2d_ops.c
    convolution_1d_ops.c
)
target_include_directories(ops PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
if(HAVE_OPENMP)
    target_link_libraries(ops PUBLIC OpenMP::OpenMP_C)
endif()

set(LIBSOMBRERO_OBJECTS $<TARGET_OBJECTS:ops>)

if(HAVE_SSE42)
    add_library(ops_sse42 OBJECT
        data_ops.c
        convolution_2d_ops.c
        convolution_1d_ops.c
    )
    target_include_directories(ops_sse42 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    set_target_properties(ops_sse42 PROPERTIES COMPILE_FLAGS "${SSE42_CFLAGS}")
    if(HAVE_OPENMP)
        target_link_libraries(ops_sse42 PUBLIC OpenMP::OpenMP_C)
    endif()
    list(APPEND LIBSOMBRERO_OBJECTS $<TARGET_OBJECTS:ops_sse42>)
endif()

if(HAVE_AVX)
    add_library(ops_avx OBJECT
        data_ops.c
        convolution_2d_ops.c
        convolution_1d_ops.c
    )
    target_include_directories(ops_avx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    set_target_properties(ops_avx PROPERTIES COMPILE_FLAGS "${AVX_CFLAGS}")
    if(HAVE_OPENMP)
        target_link_libraries(ops_avx PUBLIC OpenMP::OpenMP_C)
    endif()
    list(APPEND LIBSOMBRERO_OBJECTS $<TARGET_OBJECTS:ops_avx>)
endif()

if(HAVE_AVX2)
    add_library(ops_avx2 OBJECT
        data_ops.c
        convolution_2d_ops.c
        convolution_1d_ops.c
    )
    target_include_directories(ops_avx2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    set_target_properties(ops_avx2 PROPERTIES COMPILE_FLAGS "${AVX2_CFLAGS}")
    if(HAVE_OPENMP)
        target_link_libraries(ops_avx2 PUBLIC OpenMP::OpenMP_C)
    endif()
    list(APPEND LIBSOMBRERO_OBJECTS $<TARGET_OBJECTS:ops_avx2>)
endif()

if(HAVE_AVX512)
    add_library(ops_avx512 OBJECT
        data_ops.c
        convolution_2d_ops.c
        convolution_1d_ops.c
    )
    target_include_directories(ops_avx512 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    set_target_properties(ops_avx512 PROPERTIES COMPILE_FLAGS "${AVX512_CFLAGS}")
    if(HAVE_OPENMP)
        target_link_libraries(ops_avx512 PUBLIC OpenMP::OpenMP_C)
    endif()
    list(APPEND LIBSOMBRERO_OBJECTS $<TARGET_OBJECTS:ops_avx512>)
endif()

if(HAVE_FMA)
    add_library(ops_fma OBJECT
        data_ops.c
        convolution_2d_ops.c
        convolution_1d_ops.c
    )
    target_include_directories(ops_fma PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    set_target_properties(ops_fma PROPERTIES COMPILE_FLAGS "${FMA_CFLAGS}")
    if(HAVE_OPENMP)
        target_link_libraries(ops_fma PUBLIC OpenMP::OpenMP_C)
    endif()
    list(APPEND LIBSOMBRERO_OBJECTS $<TARGET_OBJECTS:ops_fma>)
endif()

add_library(sombrero SHARED
    convolution.c
    process.c
    wavelet.c
    noise.c
    object.c
    reconstruct.c
    cpu.c
    ${LIBSOMBRERO_OBJECTS}
)

target_include_directories(sombrero PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(sombrero PUBLIC m)
if(HAVE_OPENMP)
    target_link_libraries(sombrero PUBLIC OpenMP::OpenMP_C)
endif()

set_target_properties(sombrero PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

install(TARGETS sombrero
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)
