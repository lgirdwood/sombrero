# Tests for libsombrero

# We need the example utility code to load bmps for our tests
add_library(test_utils OBJECT ../examples/bmp.c ../examples/fits.c)
target_include_directories(test_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# test_atrous
add_executable(test_atrous test_atrous.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_atrous PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_atrous PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_atrous COMMAND test_atrous wiz-ha-x.bmp test_out_atrous)

# test_structures
add_executable(test_structures test_structures.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_structures PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_structures PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_structures COMMAND test_structures wiz-ha-x.bmp test_out_structures)

# test_objects
add_executable(test_objects test_objects.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_objects PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_objects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_objects COMMAND test_objects -i wiz-ha-x.bmp -o test_out_objects)

# test_reconstruct
add_executable(test_reconstruct test_reconstruct.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_reconstruct PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_reconstruct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_reconstruct COMMAND test_reconstruct -i wiz-ha-x.bmp -o test_out_reconstruct.bmp)

# test_performance
add_executable(test_performance test_performance.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_performance PRIVATE sombrero ${CFITSIO_LIBRARIES})
target_include_directories(test_performance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
if(ENABLE_OPENCL)
    target_compile_definitions(test_atrous PRIVATE HAVE_OPENCL)
    target_compile_definitions(test_objects PRIVATE HAVE_OPENCL)
    target_compile_definitions(test_performance PRIVATE HAVE_OPENCL)
endif()
add_test(NAME test_performance COMMAND test_performance -i wiz-ha-x.bmp -o test_out_performance)

# Copy test images and scripts to the build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/images/wiz-ha-x.bmp
               ${CMAKE_CURRENT_BINARY_DIR}/wiz-ha-x.bmp COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/images/ngc7380.fit
               ${CMAKE_CURRENT_BINARY_DIR}/ngc7380.fit COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh
               ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh COPYONLY)

# Package Validation Test
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_packages.sh
               ${CMAKE_CURRENT_BINARY_DIR}/test_packages.sh COPYONLY)
add_test(NAME test_packages COMMAND ./test_packages.sh)
