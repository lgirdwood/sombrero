# Tests for libsombrero

# We need the example utility code to load bmps for our tests
add_library(test_utils OBJECT ../examples/bmp.c)
target_include_directories(test_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# test_atrous
add_executable(test_atrous test_atrous.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_atrous PRIVATE sombrero)
target_include_directories(test_atrous PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_atrous COMMAND test_atrous wiz-ha-x.bmp test_out_atrous)

# test_structures
add_executable(test_structures test_structures.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_structures PRIVATE sombrero)
target_include_directories(test_structures PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_structures COMMAND test_structures wiz-ha-x.bmp test_out_structures)

# test_objects
add_executable(test_objects test_objects.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_objects PRIVATE sombrero)
target_include_directories(test_objects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_objects COMMAND test_objects -i wiz-ha-x.bmp -o test_out_objects)

# test_reconstruct
add_executable(test_reconstruct test_reconstruct.c $<TARGET_OBJECTS:test_utils>)
target_link_libraries(test_reconstruct PRIVATE sombrero)
target_include_directories(test_reconstruct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME test_reconstruct COMMAND test_reconstruct -i wiz-ha-x.bmp -o test_out_reconstruct.bmp)

# Copy test image and script to the build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/images/wiz-ha-x.bmp
               ${CMAKE_CURRENT_BINARY_DIR}/wiz-ha-x.bmp COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh
               ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh COPYONLY)
