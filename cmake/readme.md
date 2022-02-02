## 说明

该仓库主要为Cmake 相关工具类

* android.cmake 主要为NDK配置,使用参考如下

```cmake
#必须在project前调用
init_android_ndk("path/to/ndk_home")
#...
project("name")
#... 
#第二步调用 
init_android_build(target_abi api_level toolchain_name stl_type)
#第三步调用
config_toolchain_file()
```

* common.cmake 主要一些工具类

```cmake
#打印cmake 中所有变量信息
dump_all_variables()
#查找系统中ndk位置,结果存在result中
find_ndk_path(result)
#开启发布版本参数，非强制开启，如果构建目标为Debug版本将不开启
enable_release_build()
#强制开启发布版本参数
enforce_enable_release_build()
```

* git_utils.cmake 主要是git 相关工具类，详细使用说明参数文件中注释

* resources.cmake 主要为弥补linux 可执行文件中添加资源相关问题，使用例子如下

```cmake
cmrc_add_resource_library(gamme-resources
        ALIAS game::rc
        NAMESPACE game
        #添加资源文件路径
        src/zoo/scenes/HomeScene.lua
        )
target_link_libraries(game game::rc ...)
```

```c++
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(game);

uint8_t *getHookData(std::string name, unsigned long *pSize) {
    LOG(DEBUG) << "Patch:" << name;
    auto fs = cmrc::game::get_filesystem();
    auto file = fs.open(name);
    uint64_t len = file.end() - file.begin();
    auto *data_ = new uint8_t[len];
    memcpy(data_, file.begin(), len);
    *pSize = file.end() - file.begin();
    return data_;
}

```

* vcpkg.android.cmake 以NDK和vcpkg结合的相工具类，使用说明如下

```cmake
#必须在project前调用
init_vcpkg_android_ndk("path/to/ndk_home")
#...
project("name")
#... 
#第二步调用 
init_vcpkg_android_build(target_abi api_level toolchain_name stl_type)
#第三步调用
config_vcpkg_toolchain_file()
```

* vcpkg_utils.cmake vcpkg 相关工具类

```cmake
# 设置cmake环境上下文代理
set_proxy_env(host port)

```
