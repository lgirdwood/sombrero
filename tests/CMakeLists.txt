# Tests for libsombrero

# We need the example utility code to load bmps for our tests
add_library(test_utils OBJECT ../examples/bmp.c ../examples/fits.c)
target_include_directories(test_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# test_atrous
add_executable(test_atrous test_atrous.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_atrous PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_atrous PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# test_structures
add_executable(test_structures test_structures.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_structures PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_structures PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# test_objects
add_executable(test_objects test_objects.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_objects PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_objects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# test_reconstruct
add_executable(test_reconstruct test_reconstruct.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_reconstruct PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_reconstruct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# test_performance
add_executable(test_performance test_performance.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_performance PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_performance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
if(ENABLE_OPENCL)
    target_compile_definitions(test_atrous PRIVATE HAVE_OPENCL)
    target_compile_definitions(test_objects PRIVATE HAVE_OPENCL)
    target_compile_definitions(test_performance PRIVATE HAVE_OPENCL)
endif()

# Define test images
set(TEST_IMAGES 
    wiz-ha-x.bmp 
    ngc7380.fit 
    skv1427378808925.fits 
    skv2320143407510.fits)

foreach(IMG IN LISTS TEST_IMAGES)
    get_filename_component(IMG_NAME ${IMG} NAME_WE)
    
    # Copy test images to the build directory
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/images/${IMG}
                   ${CMAKE_CURRENT_BINARY_DIR}/${IMG} COPYONLY)

    add_test(NAME test_atrous_${IMG_NAME} COMMAND test_atrous ${IMG} test_out_atrous_${IMG_NAME})
    add_test(NAME test_structures_${IMG_NAME} COMMAND test_structures ${IMG} test_out_structures_${IMG_NAME})
    add_test(NAME test_objects_${IMG_NAME} COMMAND test_objects -i ${IMG} -o test_out_objects_${IMG_NAME})
    add_test(NAME test_reconstruct_${IMG_NAME} COMMAND test_reconstruct -i ${IMG} -o test_out_reconstruct_${IMG_NAME}.bmp)
    add_test(NAME test_performance_${IMG_NAME} COMMAND test_performance -i ${IMG} -o test_out_performance_${IMG_NAME})
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh
               ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh COPYONLY)

# Package Validation Test
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_packages.sh
               ${CMAKE_CURRENT_BINARY_DIR}/test_packages.sh @ONLY)
add_test(NAME test_packages COMMAND ./test_packages.sh)
