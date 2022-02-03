//
// Created by xiaobai on 2022/2/3.
//

#ifndef NDK_MEMORY_LEAK_TRACKER_MEMORY_TRACER_H
#define NDK_MEMORY_LEAK_TRACKER_MEMORY_TRACER_H

#include "memory_utils.h"
#include "hashmap.h"

class MemoryTracer {
public:
    static MemoryTracer *getInstance();


    void setEnablePss(bool enablePss);

    void setEnableVss(bool enableVss);

    void startMonitoringThisThread();

    void startMonitoringAllThreads();

    void stopMonitoringThisThread();

    void stopAllMonitoring();

    bool saveLeakData(const char *path);

    bool dumpLeakInfo();

    void cleanInfo();

private:
    MemoryTracer();

private://for record
    void recordPss(void *ptr, size_t length, Backtrace *backtrace);

    void recordVss(void *ptr, size_t length, Backtrace *backtrace);

    void remove(void *ptr);

    void insert(uintptr_t address, size_t size, Backtrace *backtrace);

    static bool hashCallbackFile(void *key, void *value, void *context);

    static bool dumpLeakCallback(void *key, void *value, void *context);

    static bool cleanEntry(void *key, void *value, void *context);


private://for proxy
    static void *malloc_proxy(size_t __byte_count);

    static void *calloc_proxy(size_t count, size_t bytes);

    static void free_proxy(void *address);

    static void *mmap_proxy(void *ptr, size_t size, int port, int flags, int fd, off_t offset);

    static void *mmap64_proxy(void *ptr, size_t size, int port, int flags, int fd, off64_t offset);

    static int munmap_proxy(void *address, size_t size);

    static void *memalign_proxy(size_t alignment, size_t size);

    static void *realloc_proxy(void *ptr, size_t size);

    static char *strdup_proxy(const char *str);

private:
    static const void *sPltGot[][2];
    static pthread_key_t guard;
private:
    bool enable_pss = false;
    bool enable_vss = false;
    bool only_record_this_thread = false;
    pthread_t record_thread = -1;
    Hashmap *hashmap = nullptr;
private:
    bool initPltHook();

};


#endif //NDK_MEMORY_LEAK_TRACKER_MEMORY_TRACER_H
