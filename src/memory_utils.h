//
// Created by xiaobai on 2022/2/3.
//

#ifndef NDK_MEMORY_LEAK_TRACKER_MEMORY_UTILS_H
#define NDK_MEMORY_LEAK_TRACKER_MEMORY_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, __FUNCTION__, __VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, __VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,__FUNCTION__ ,__VA_ARGS__)
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,__FUNCTION__ ,__VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,__FUNCTION__ ,__VA_ARGS__)

#define MAX_TRACE_DEPTH 16
#define MAX_BUFFER_SIZE 1024

#define ALLOC_INDEX_SIZE 1 << 16
#define ALLOC_CACHE_SIZE 1 << 15

typedef struct {
    uint32_t depth;
    uintptr_t trace[MAX_TRACE_DEPTH];
} Backtrace;

struct AllocNode {
    uint32_t size;
    uintptr_t addr;
    uintptr_t trace[MAX_TRACE_DEPTH];
};


char *get_self_elf_path();

void *real_malloc(size_t __byte_count);

void real_free(void *ptr);

void *real_calloc(size_t __item_count, size_t __item_size);

void *real_realloc(void *ptr, size_t size);

void *real_memalign(size_t alignment, size_t size);

void *real_mmap(void *ptr, size_t size, int port, int flags, int fd, off_t offset);

void *real_mmap64(void *ptr, size_t size, int port, int flags, int fd, off64_t offset);

int real_munmap(void *address, size_t size);

char *real_strdup(const char *str);


#ifdef __cplusplus
}
#endif
#endif //NDK_MEMORY_LEAK_TRACKER_MEMORY_UTILS_H
