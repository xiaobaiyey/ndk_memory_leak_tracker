

add_executable(test test.cpp)
target_link_libraries(test ndk_memory_leak_tracker log)