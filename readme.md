## Memory Leak Check For NDK

## Get started

Step 1: Add the static library to your CMakeLists.txt

```cmake
include_directories(./src)
target_link_libraries({your_library} ndk_memory_leak_tracker log)
```

Step 2: Add code for simple usage

```c++
auto instance = MemoryTracer::getInstance();
//start monitor
instance->startMonitoringThisThread();
/**
 * ......
 * ......
 * ......
 * some code to check
 */
//stop monitor
instance->stopMonitoringThisThread();
//print message to logcat
instance->dumpLeakInfo();
//save leak info to file
instance->saveLeakData("/data/local/tmp/trace.log");
```

Step 3: Get memory leak details info by ndk-stack

```shell
## get by logcat
adb logcat | ndk-stack -sym ./build/bin/x86_64
```

```shell
## get by file
ndk-stack -sym ./build/bin/x86_64 -i trace.log
```

detail:

```
********** Crash dump: **********
Build fingerprint: 'Android/sdk_phone64_x86_64/emulator64_x86_64:12/SC/7752726:userdebug/test-keys'
#00 0x0000000000008b25 /data/local/tmp/test (captureBacktrace(void**, unsigned long)+69)
                                             captureBacktrace(void**, unsigned long)
                                             C:/Users/xiaobai/CLionProjects/ndk-memory-leak-tracker/src\memory_tracer.cpp:103:18
#01 0x0000000000008126 /data/local/tmp/test (MemoryTracer::malloc_proxy(unsigned long)+166)
                                             MemoryTracer::malloc_proxy(unsigned long)
                                             C:/Users/xiaobai/CLionProjects/ndk-memory-leak-tracker/src\memory_tracer.cpp:148:29
#02 0x0000000000007f62 /data/local/tmp/test (malloc_test()+18)
                                             malloc_test()
                                             C:/Users/xiaobai/CLionProjects/ndk-memory-leak-tracker/test\test.cpp:18:11
#03 0x0000000000007f99 /data/local/tmp/test (bar()+9)
                                             bar()
                                             C:/Users/xiaobai/CLionProjects/ndk-memory-leak-tracker/test\test.cpp:26:1
#04 0x0000000000007fa9 /data/local/tmp/test (foo()+9)
                                             foo()
                                             C:/Users/xiaobai/CLionProjects/ndk-memory-leak-tracker/test\test.cpp:32:1
#05 0x0000000000008046 /data/local/tmp/test (main+38)
                                             main
                                             C:/Users/xiaobai/CLionProjects/ndk-memory-leak-tracker/test\test.cpp:53:5
#06 0x000000000005007a /apex/com.android.runtime/lib64/bionic/libc.so (__libc_init+90)
Crash dump is completed
```

## others
1. [bytedance/memory-leak-detector](https://github.com/bytedance/memory-leak-detector)
2. [iqiyi/xHook](https://github.com/iqiyi/xHook)
3. [hexhacking/xDL](https://github.com/hexhacking/xDL)
