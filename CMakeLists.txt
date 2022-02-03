cmake_minimum_required(VERSION 3.16)
set(CMAKE_BUILD_TYPE Debug)
#判断目标文件夹是否存在，存在则引用相关设置
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/cmake)
    include(cmake/android.cmake)
    #根据host 设置ndk
    if (WIN32)
        #init_android_ndk("E:/android_sdk/ndk/21.1.6352462")
        init_android_ndk("D:/sdk/ndk/21.3.6528147")
    else ()
        init_android_ndk("/mnt/c/android-ndk-r21b/")
    endif ()
else ()
    message(FATAL_ERROR "can not find cmake tools dir,please clone cmake tools")
endif ()
# target_abi must in [armeabi-v7a、arm64-v8a、x86、x86_64]
# toolchain_name must in [gcc、clang]
# stl_type must in [c++_static、c++_shared、none、system]
init_android_build("x86_64" 19 "clang" "c++_static")
project(ndk_memory_leak_tracker)

config_toolchain_file()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/staticlibs/${ANDROID_ABI})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/jnilibs/${ANDROID_ABI})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${ANDROID_ABI})

set(CMAKE_CXX_STANDARD 11)


set(xhook_src
        xHook/libxhook/jni/xh_core.c
        xHook/libxhook/jni/xh_elf.c
        xHook/libxhook/jni/xh_jni.c
        xHook/libxhook/jni/xh_log.c
        xHook/libxhook/jni/xh_util.c
        xHook/libxhook/jni/xh_version.c
        xHook/libxhook/jni/xhook.c
        )

set(xdl_src
        xDL/xdl/src/main/cpp/xdl.c
        xDL/xdl/src/main/cpp/xdl_iterate.c
        xDL/xdl/src/main/cpp/xdl_linker.c
        xDL/xdl/src/main/cpp/xdl_lzma.c
        xDL/xdl/src/main/cpp/xdl_util.c
        )
include_directories(xHook/libxhook/jni xDL/xdl/src/main/cpp/include)

aux_source_directory(src leak_check_src)

add_library(ndk_memory_leak_tracker ${xhook_src} ${xdl_src} ${leak_check_src})
add_subdirectory(test)