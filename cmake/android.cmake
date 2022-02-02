
#[[
全局变量设置
set_property(GLOBAL PROPERTY source_list_property "${source_list}")
]]

# target_abi must in [armeabi-v7a、arm64-v8a、x86、x86_64]
# toolchain_name must in [gcc、clang]
# stl_type must in [c++_static、c++_shared、none、system]
set(target_abi_list armeabi-v7a arm64-v8a x86 x86_64 "armeabi-v7a with NEON")
set(toolchain_name_list gcc clang)
set(stl_type_list c++_static c++_shared none system gnustl_static)

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)

# 设置ndk android.toolchain.cmake 文件夹位置
# 当前函数必须在project()函数前调用，否在CMAKE_TOOLCHAIN_FILE 不生效
function(_set_android_toolchain_file toolchain_file)
    if (EXISTS ${NDK_HOME})
        set(CMAKE_TOOLCHAIN_FILE ${NDK_HOME}/build/cmake/android.toolchain.cmake CACHE STRING "for android.toolchain.cmake path")
    endif ()
    if (IS_DIRECTORY ${toolchain_file})
        if (EXISTS ${toolchain_file}/build/cmake/android.toolchain.cmake)
            set(CMAKE_TOOLCHAIN_FILE ${toolchain_file}/build/cmake/android.toolchain.cmake CACHE STRING "for android.toolchain.cmake path")
            message("-- set CMAKE_TOOLCHAIN_FILE to:${CMAKE_TOOLCHAIN_FILE}")
            return()
        endif ()
    endif ()
    if (EXISTS ${toolchain_file})
        get_filename_component(toolchain_file_name ${toolchain_file} NAME)
        if (${toolchain_file_name} STREQUAL "android.toolchain.cmake")
            set(CMAKE_TOOLCHAIN_FILE ${toolchain_file}/build/cmake/android.toolchain.cmake CACHE STRING "for android.toolchain.cmake path")
            message("-- set CMAKE_TOOLCHAIN_FILE to:${CMAKE_TOOLCHAIN_FILE}")
            return()
        endif ()
    endif ()
    message(WARNING "toolchain_file:${toolchain_file} is not android toolchain file,try find ndk home by env")
    find_ndk_path(android_ndk_home)
    if (EXISTS ${android_ndk_home}/build/cmake/android.toolchain.cmake)
        set(CMAKE_TOOLCHAIN_FILE ${toolchain_file}/build/cmake/android.toolchain.cmake CACHE STRING "for android.toolchain.cmake path")
        message("-- set CMAKE_TOOLCHAIN_FILE to:${CMAKE_TOOLCHAIN_FILE}")
        return()
    endif ()
    message(FATAL_ERROR "-- can not find android.toolchain.cmake file,please set [toolchain_file] to NDK home or android.toolchain.cmake")
endfunction()


# 设置ndk目录信息
function(_set_ndk_home _ndk_home)
    if (IS_DIRECTORY ${_ndk_home})
        if (EXISTS ${_ndk_home}/ndk-build.cmd OR EXISTS ${ndk_home}/ndk-build)
            set(ANDROID_NDK_HOME ${_ndk_home} CACHE STRING "for ndk home path")
            set_property(GLOBAL PROPERTY ANDROID_NDK_HOME ${_ndk_home})
            message("-- set NDK_HOME to: ${ANDROID_NDK_HOME}")
        endif ()
    else ()
        message(FATAL_ERROR "-- NDK_HOME:${_ndk_home} not a directory")
    endif ()
endfunction()


function(init_android_ndk ndk_home)
    _set_ndk_home(${ndk_home})
    _set_android_toolchain_file(${ndk_home})
    file(READ "${ndk_home}/source.properties" ANDROID_NDK_SOURCE_PROPERTIES)
    set(ANDROID_NDK_REVISION_REGEX
            "^Pkg\\.Desc = Android NDK\nPkg\\.Revision = ([0-9]+)\\.([0-9]+)\\.([0-9]+)(-beta([0-9]+))?")
    if (NOT ANDROID_NDK_SOURCE_PROPERTIES MATCHES "${ANDROID_NDK_REVISION_REGEX}")
        message(SEND_ERROR "-- Failed to parse Android NDK revision: ${ANDROID_NDK}/source.properties.\n${ANDROID_NDK_SOURCE_PROPERTIES}")
    endif ()
    message("-- Android NDK Major: ${CMAKE_MATCH_1}")
    message("-- Android NDK Minor: ${CMAKE_MATCH_2}")
    message("-- Android NDK Build: ${CMAKE_MATCH_3}")
endfunction()


#[[
此处使用macro 而不是使用function 因为function变量设置存在问题
]]
macro(config_toolchain_file)
    if (ANDROID_ABI STREQUAL armeabi-v7a OR ANDROID_ABI STREQUAL "armeabi-v7a with NEON")
        set(ANDROID_COMPILER_PREFIX armv7a-linux-androideabi${ANDROID_NATIVE_API_LEVEL})
    elseif (ANDROID_ABI STREQUAL arm64-v8a)
        set(ANDROID_COMPILER_PREFIX aarch64-linux-android${ANDROID_NATIVE_API_LEVEL})
    elseif (ANDROID_ABI STREQUAL x86)
        #i686-linux-android23
        set(ANDROID_COMPILER_PREFIX i686-linux-android${ANDROID_NATIVE_API_LEVEL})
    elseif (ANDROID_ABI STREQUAL x86_64)
        #x86_64-linux-android22
        set(ANDROID_COMPILER_PREFIX x86_64-linux-android${ANDROID_NATIVE_API_LEVEL})
    else ()
        message("-- Invalid Android ABI: ${ANDROID_ABI}")
    endif ()
    message("-- ANDROID_COMPILER_PREFIX:${ANDROID_COMPILER_PREFIX}")
    if (ANDROID_NDK_MAJOR VERSION_LESS 19)
        message(STATUS "ndk version < 19")
        add_compile_options(--target=${ANDROID_COMPILER_PREFIX})
        set(CMAKE_CXX_FLAGS "--target=${ANDROID_COMPILER_PREFIX} ${CMAKE_CXX_FLAGS}")
    else ()
        #不同系统上的 android ndk 存在形式
        if (${CMAKE_HOST_SYSTEM} MATCHES Windows)
            set(CMAKE_C_COMPILER ${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_COMPILER_PREFIX}-clang.cmd)
            message("-- set CMAKE_C_COMPILER:${CMAKE_C_COMPILER}")
            set(CMAKE_CXX_COMPILER ${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_COMPILER_PREFIX}-clang++.cmd)
            message("-- set CMAKE_CXX_COMPILER:${CMAKE_CXX_COMPILER}")
        else ()
            set(CMAKE_C_COMPILER ${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_COMPILER_PREFIX}-clang)
            set(CMAKE_CXX_COMPILER ${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_COMPILER_PREFIX}-clang++)
        endif ()
    endif ()
endmacro()


function(init_android_build target_abi api_level toolchain_name stl_type)
    set(CMAKE_SYSTEM_NAME "Android" CACHE STRING "for CMAKE_SYSTEM_NAME value")
    if (NOT ${target_abi} IN_LIST target_abi_list)
        message(FATAL_ERROR "-- unsupported target abi:${target_abi},target_abi must in ${target_abi_list}")
    endif ()
    #设置目标架构
    set(ANDROID_ABI ${target_abi} CACHE STRING "for ANDROID_ABI value")
    if (NOT ${toolchain_name} IN_LIST toolchain_name_list)
        message(FATAL_ERROR "-- unsupported toolchain name:${toolchain_name},toolchain name must in ${toolchain_name_list}")
    endif ()
    set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION ${toolchain_name} CACHE STRING "for CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION value")
    if (NOT ${stl_type} IN_LIST stl_type_list)
        message(FATAL_ERROR "-- unsupported stl type :${stl_type},stl type must in ${stl_type_list}")
    endif ()
    set(CMAKE_ANDROID_STL_TYPE ${stl_type} CACHE STRING "for CMAKE_ANDROID_STL_TYPE value")
    set(ANDROID_STL ${stl_type} CACHE STRING "for CMAKE_ANDROID_STL_TYPE value")
    set(ANDROID_NATIVE_API_LEVEL ${api_level} CACHE STRING "for ANDROID_NATIVE_API_LEVEL value")
    set(ANDROID_PLATFORM "android-${api_level}" CACHE STRING "for ANDROID_PLATFORM value")
endfunction()


#[[
设置输出文件目录
]]
macro(unite_build_output)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/staticlibs/${ANDROID_ABI})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/jnilibs/${ANDROID_ABI})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${ANDROID_ABI})
endmacro()


