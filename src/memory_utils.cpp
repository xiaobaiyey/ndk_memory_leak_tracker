//
// Created by xiaobai on 2022/2/3.
//

#include "memory_utils.h"
#include <string>
#include <dlfcn.h>

static void *(*malloc_origin)(size_t) = nullptr;

static void *(*calloc_origin)(size_t, size_t) = nullptr;

static void *(*realloc_origin)(void *, size_t) = nullptr;

static void *(*memalign_origin)(size_t, size_t) = nullptr;

static void (*free_origin)(void *) = nullptr;

static void *(*mmap_origin)(void *, size_t, int, int, int, off_t) = nullptr;

static void *(*mmap64_origin)(void *, size_t, int, int, int, off64_t) = nullptr;

static int (*munmap_origin)(void *, size_t) = nullptr;

static char *(*strdup_origin)(const char *) = nullptr;


static void ForeachMemoryRange(std::function<bool(uintptr_t, uintptr_t, char *, char *)> callback) {
    FILE *f;
    if ((f = fopen("/proc/self/maps", "r"))) {
        char buf[PATH_MAX], perm[12] = {'\0'}, dev[12] = {'\0'}, mapname[PATH_MAX] = {'\0'};
        uintptr_t begin, end, inode, foo;

        while (!feof(f)) {
            if (fgets(buf, sizeof(buf), f) == 0)
                break;
            sscanf(buf, "%lx-%lx %s %lx %s %ld %s", &begin, &end, perm,
                   &foo, dev, &inode, mapname);
            if (!callback(begin, end, perm, mapname)) {
                break;
            }
        }
        fclose(f);
    }
}

static bool init_proxy() {
    void *handler = dlopen("libc.so", 0);
    if (handler == nullptr) {
        LOGE("dlopen libc.so fail");
        return false;
    }
    malloc_origin = (void *(*)(size_t)) dlsym(handler, "malloc");
    if (malloc_origin == nullptr) {
        LOGE("dlsym malloc fail");
        return false;
    }
    calloc_origin = (void *(*)(size_t, size_t)) dlsym(handler, "calloc");
    if (calloc_origin == nullptr) {
        LOGE("dlsym calloc fail");
        return false;
    }
    realloc_origin = (void *(*)(void *, size_t)) dlsym(handler, "realloc");
    if (realloc_origin == nullptr) {
        LOGE("dlsym realloc fail");
        return false;
    }
    memalign_origin = (void *(*)(size_t, size_t)) dlsym(handler, "memalign");
    if (memalign_origin == nullptr) {
        LOGE("dlsym memalign fail");
        return false;
    }
    free_origin = (void (*)(void *)) dlsym(handler, "free");
    if (free_origin == nullptr) {
        LOGE("dlsym free fail");
        return false;
    }
    mmap_origin = (void *(*)(void *, size_t, int, int, int, off_t)) dlsym(handler, "mmap");
    if (mmap_origin == nullptr) {
        LOGE("dlsym mmap fail");
        return false;
    }
    mmap64_origin = (void *(*)(void *, size_t, int, int, int, off64_t)) dlsym(handler, "mmap64");
    if (mmap64_origin == nullptr) {
        LOGE("dlsym mmap64 fail");
        return false;
    }
    munmap_origin = (int (*)(void *, size_t)) dlsym(handler, "munmap");
    if (munmap_origin == nullptr) {
        LOGE("dlsym munmap fail");
        return false;
    }
    strdup_origin = (char *(*)(const char *)) dlsym(handler, "strdup");
    if (strdup_origin == nullptr) {
        LOGE("dlsym strdup fail");
        return false;
    }
    return true;
}

/**
 *
 * @return need free by self
 */
char *get_self_elf_path() {
    std::string self_elf_path;
    uintptr_t address = (uintptr_t) &(ForeachMemoryRange);
    auto fun = [&](uintptr_t start, uintptr_t end, char *perm, char *mapname) -> bool {
        if (address >= start && address <= end) {
            self_elf_path = mapname;
            return false;
        }
        return true;
    };
    ForeachMemoryRange(fun);
    return strdup(self_elf_path.c_str());
}


void *real_malloc(size_t __byte_count) {
    if (malloc_origin == nullptr) {
        init_proxy();
    }
    return malloc_origin(__byte_count);
}

void real_free(void *ptr) {
    if (free_origin == nullptr) {
        init_proxy();
    }
    return free_origin(ptr);
}

void *real_calloc(size_t __item_count, size_t __item_size) {
    if (calloc_origin == nullptr) {
        init_proxy();
    }
    return calloc_origin(__item_count, __item_size);
}

void *real_realloc(void *ptr, size_t size) {
    if (realloc_origin == nullptr) {
        init_proxy();
    }
    return realloc_origin(ptr, size);
}

void *real_memalign(size_t alignment, size_t size) {
    if (memalign_origin == nullptr) {
        init_proxy();
    }
    return memalign_origin(alignment, size);
}

void *real_mmap(void *ptr, size_t size, int port, int flags, int fd, off_t offset) {
    if (mmap_origin == nullptr) {
        init_proxy();
    }
    return mmap_origin(ptr, size, port, flags, fd, offset);
}

void *real_mmap64(void *ptr, size_t size, int port, int flags, int fd, off64_t offset) {
    if (mmap64_origin == nullptr) {
        init_proxy();
    }
    return mmap64_origin(ptr, size, port, flags, fd, offset);
}

int real_munmap(void *address, size_t size) {
    if (munmap_origin == nullptr) {
        init_proxy();
    }
    return munmap_origin(address, size);
}

char *real_strdup(const char *str) {
    if (strdup_origin == nullptr) {
        init_proxy();
    }
    return strdup_origin(str);
}
