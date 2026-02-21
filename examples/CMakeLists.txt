add_executable(smbrr-atrous atrous.c bmp.c)
target_link_libraries(smbrr-atrous PRIVATE sombrero)
target_include_directories(smbrr-atrous PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(smbrr-structures structures.c bmp.c)
target_link_libraries(smbrr-structures PRIVATE sombrero)
target_include_directories(smbrr-structures PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(smbrr-objects objects.c bmp.c)
target_link_libraries(smbrr-objects PRIVATE sombrero)
target_include_directories(smbrr-objects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(smbrr-reconstruct reconstruct.c bmp.c)
target_link_libraries(smbrr-reconstruct PRIVATE sombrero)
target_include_directories(smbrr-reconstruct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(smbrr-reconstruct-1d reconstruct-1d.c)
target_link_libraries(smbrr-reconstruct-1d PRIVATE sombrero)
target_include_directories(smbrr-reconstruct-1d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
