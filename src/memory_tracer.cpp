//
// Created by xiaobai on 2022/2/3.
//

#include <cstring>
#include "memory_tracer.h"
#include "memory_utils.h"
#include "xhook.h"
#include "xdl.h"
#include <unwind.h>
#include <thread>
#include <sys/mman.h>
#include <cxxabi.h>

#define LOGDT(TAG, ...)  __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__)
#if defined(__LP64__)
#define STACK_FORMAT_HEADER "\n0x%016lx, %u, 1\n"
#define STACK_FORMAT_UNKNOWN "\t\t #%02d pc %016lx <unknown>\n"
#define STACK_FORMAT_ANONYMOUS "\t\t #%02d pc %016lx <anonymous:%016lx>\n"
#define STACK_FORMAT_FILE "\t\t#%02d pc %016lx %s (unknown)\n"
#define STACK_FORMAT_FILE_NAME "\t\t #%02d pc %016lx  %s (%s+\?)\n"
#define STACK_FORMAT_FILE_NAME_LINE "\t\t #%02d pc %016lx  %s (%s+%lu)\n"
#else
#define STACK_FORMAT_HEADER "\n0x%08x, %u, 1\n"
#define STACK_FORMAT_UNKNOWN "\t\t #%02d pc %08x <unknown>\n"
#define STACK_FORMAT_ANONYMOUS "\t\t #%02d pc %08x <anonymous:%08x>\n"
#define STACK_FORMAT_FILE "\t\t #%02d pc %08x %s (unknown)\n"
#define STACK_FORMAT_FILE_NAME "\t\t #%02d pc %08x %s  (%s + \?)\n"
#define STACK_FORMAT_FILE_NAME_LINE "\t\t #%02d pc %08x %s  (%s + %u)\n"
#endif
static volatile bool init_success = false;

pthread_key_t MemoryTracer::guard;

const void *MemoryTracer::sPltGot[][2] = {
        {
                "malloc",
                (void *) malloc_proxy
        },
        {
                "calloc",
                (void *) calloc_proxy
        },
        {
                "realloc",
                (void *) realloc_proxy
        },
        {
                "memalign",
                (void *) memalign_proxy
        },
        {
                "free",
                (void *) free_proxy
        },
        {
                "mmap",
                (void *) mmap_proxy
        },
        {
                "mmap64",
                (void *) mmap64_proxy
        },
        {
                "munmap",
                (void *) munmap_proxy
        },
        {
                "strdup",
                (void *) strdup_proxy
        }
};

struct BacktraceState {
    void **current;
    void **end;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg) {
    BacktraceState *state = static_cast<BacktraceState *>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void *>(pc);
        }
    }
    return _URC_NO_REASON;
}

static size_t hashmapUintPtrHash(void *key) {
    // Return the key value itself.
    return (uintptr_t) key;
}

static bool hashmapUintPtrEquals(void *keyA, void *keyB) {
    auto a = (uintptr_t) keyA;
    auto b = (uintptr_t) keyB;
    return a == b;
}


size_t captureBacktrace(void **buffer, size_t max) {
    BacktraceState state = {buffer, buffer + max};
    _Unwind_Backtrace(unwindCallback, &state);
    return state.current - buffer;
}

MemoryTracer *MemoryTracer::getInstance() {
    static MemoryTracer memoryTracer;
    return &memoryTracer;
}

MemoryTracer::MemoryTracer() {
    this->hashmap = hashmapCreate(1 << 15, hashmapUintPtrHash, hashmapUintPtrEquals);
    cleanInfo();
    if (not initPltHook()) {
        LOGE("init plt hook fail");
        exit(-1);
    }
    pthread_key_create(&guard, nullptr);
    init_success = true;
}

bool MemoryTracer::initPltHook() {
    auto elf_path = get_self_elf_path();
    if (elf_path == nullptr || strlen(elf_path) == 0) {
        LOGE("get self elf path fail");
        return false;
    }
    xhook_enable_debug(1);
    const int PROXY_MAPPING_LENGTH = sizeof(sPltGot) / sizeof(sPltGot[0]);
    for (int i = 0; i < PROXY_MAPPING_LENGTH; i++) {
        if (0 !=
            xhook_register(elf_path, (const char *) sPltGot[i][0], (void *) sPltGot[i][1], NULL)) {
            LOGE("register focused failed: %s, %s", elf_path, (const char *) sPltGot[i][0]);
        }
    }
    xhook_refresh(0);
    real_free(elf_path);
    return true;
}

char *MemoryTracer::strdup_proxy(const char *str) {
    if (init_success && !(uintptr_t) pthread_getspecific(guard)) {
        auto instance = MemoryTracer::getInstance();
        pthread_setspecific(guard, (void *) 1);
        char *address = real_strdup(str);
        if (address != nullptr) {
            Backtrace backtrace;
            backtrace.depth = captureBacktrace((void **) &backtrace.trace, MAX_TRACE_DEPTH);
            instance->recordPss(address, strlen(address), &backtrace);
        }
        pthread_setspecific(guard, (void *) 0);
        return address;
    } else {
        return real_strdup(str);
    }
    return nullptr;
}

void *MemoryTracer::malloc_proxy(size_t size) {
    if (init_success && !(uintptr_t) pthread_getspecific(guard)) {
        auto instance = MemoryTracer::getInstance();
        pthread_setspecific(guard, (void *) 1);
        void *address = real_malloc(size);
        if (address != nullptr) {
            Backtrace backtrace;
            backtrace.depth = captureBacktrace((void **) &backtrace.trace, MAX_TRACE_DEPTH);
            instance->recordPss(address, size, &backtrace);
        }
        pthread_setspecific(guard, (void *) 0);
        return address;
    } else {
        return real_malloc(size);
    }
}

void *MemoryTracer::calloc_proxy(size_t count, size_t bytes) {
    uint size = count * bytes;
    if (init_success && !(uintptr_t) pthread_getspecific(guard)) {
        pthread_setspecific(guard, (void *) 1);
        auto instance = MemoryTracer::getInstance();
        void *address = real_calloc(count, bytes);
        if (address != nullptr) {
            Backtrace backtrace;
            backtrace.depth = captureBacktrace((void **) &backtrace.trace, MAX_TRACE_DEPTH);
            instance->recordPss(address, size, &backtrace);
        }
        pthread_setspecific(guard, (void *) 0);
        return address;
    } else {
        return real_calloc(count, bytes);
    }
}


void MemoryTracer::recordPss(void *address, size_t size, Backtrace *backtrace) {
    if (only_record_this_thread) {
        if (record_thread != pthread_self()) {
            return;
        }
    }
    if (enable_pss) {
        insert((uintptr_t) address, size, backtrace);
    }
}

void MemoryTracer::insert(uintptr_t address, size_t size, Backtrace *backtrace) {
    AllocNode *p = static_cast<AllocNode *>(real_malloc(sizeof(AllocNode)));
    p->addr = address;
    p->size = size;
    uint depth = backtrace->depth;
    memcpy(p->trace, backtrace->trace, depth * sizeof(uintptr_t));
    p->trace[depth] = 0;
    hashmapPut(hashmap, (void *) address, p);
}

void MemoryTracer::recordVss(void *address, size_t size, Backtrace *backtrace) {
    if (only_record_this_thread) {
        if (record_thread != pthread_self()) {
            return;
        }
    }
    if (enable_vss) {
        insert((uintptr_t) address, size, backtrace);
    }
}

void *MemoryTracer::mmap_proxy(void *ptr, size_t size, int port, int flags, int fd, off_t offset) {
    if (init_success && !(uintptr_t) pthread_getspecific(guard)) {
        pthread_setspecific(guard, (void *) 1);
        auto instance = MemoryTracer::getInstance();
        void *address = real_mmap(ptr, size, port, flags, fd, offset);
        if (address != MAP_FAILED) {
            Backtrace backtrace;
            backtrace.depth = captureBacktrace((void **) &backtrace.trace, MAX_TRACE_DEPTH);
            instance->recordVss(address, size, &backtrace);
        }
        pthread_setspecific(guard, (void *) 0);
        return address;
    } else {
        return real_mmap(ptr, size, port, flags, fd, offset);
    }
}

void *MemoryTracer::mmap64_proxy(void *ptr, size_t size, int port, int flags, int fd, off64_t offset) {
    if (init_success && !(uintptr_t) pthread_getspecific(guard)) {
        pthread_setspecific(guard, (void *) 1);
        auto instance = MemoryTracer::getInstance();
        void *address = real_mmap64(ptr, size, port, flags, fd, offset);
        if (address != MAP_FAILED) {
            Backtrace backtrace;
            backtrace.depth = captureBacktrace((void **) &backtrace.trace, MAX_TRACE_DEPTH);
            instance->recordVss(address, size, &backtrace);
        }
        pthread_setspecific(guard, (void *) 0);
        return address;
    } else {
        return real_mmap64(ptr, size, port, flags, fd, offset);
    }
}

void MemoryTracer::free_proxy(void *address) {
    if (init_success && address && !(uintptr_t) pthread_getspecific(guard)) {
        pthread_setspecific(guard, (void *) 1);
        auto instance = MemoryTracer::getInstance();
        real_free(address);
        instance->remove(address);
        pthread_setspecific(guard, (void *) 0);
    } else {
        real_free(address);
    }
}

void MemoryTracer::remove(void *ptr) {
    if (only_record_this_thread) {
        if (record_thread != pthread_self()) {
            return;
        }
    }
    if ((enable_pss | enable_vss) && ptr) {
        auto value = hashmapGet(hashmap, ptr);
        if (value != nullptr) {
            real_free(value);
        }
        hashmapRemove(hashmap, ptr);
    }
}

int MemoryTracer::munmap_proxy(void *address, size_t size) {
    if (init_success && address && !(uintptr_t) pthread_getspecific(guard)) {
        pthread_setspecific(guard, (void *) 1);
        auto instance = MemoryTracer::getInstance();
        int result = real_munmap(address, size);
        if (result == 0) {
            instance->remove(address);
        }
        pthread_setspecific(guard, (void *) 0);
        return result;
    } else {
        return real_munmap(address, size);
    }
}

void *MemoryTracer::memalign_proxy(size_t alignment, size_t size) {
    if (init_success && !(uintptr_t) pthread_getspecific(guard)) {
        pthread_setspecific(guard, (void *) 1);
        auto instance = MemoryTracer::getInstance();
        void *address = real_memalign(alignment, size);
        if (address != nullptr) {
            Backtrace backtrace;
            backtrace.depth = captureBacktrace((void **) &backtrace.trace, MAX_TRACE_DEPTH);
            instance->recordPss(address, size, &backtrace);
        }
        pthread_setspecific(guard, (void *) 0);
        return address;
    } else {
        return real_memalign(alignment, size);
    }
}

void *MemoryTracer::realloc_proxy(void *ptr, size_t size) {
    if (init_success && !(uintptr_t) pthread_getspecific(guard)) {
        pthread_setspecific(guard, (void *) 1);
        auto instance = MemoryTracer::getInstance();
        void *address = real_realloc(ptr, size);
        if (ptr != nullptr && (size == 0 || address != nullptr)) {
            instance->remove(ptr);
        }
        if (address != nullptr) {
            Backtrace backtrace;
            backtrace.depth = captureBacktrace((void **) &backtrace.trace, MAX_TRACE_DEPTH);
            instance->recordPss(address, size, &backtrace);
        }
        pthread_setspecific(guard, (void *) 0);
        return address;
    } else {
        return real_realloc(ptr, size);
    }
}

void MemoryTracer::setEnablePss(bool enablePss) {
    this->enable_pss = enablePss;
}

void MemoryTracer::setEnableVss(bool enableVss) {
    this->enable_vss = enableVss;
}

void MemoryTracer::startMonitoringThisThread() {
    LOGE("startMonitoringThisThread");
    pthread_setspecific(guard, (void *) 1);
    cleanInfo();
    this->record_thread = pthread_self();
    this->only_record_this_thread = true;
    this->enable_pss = true;
    this->enable_vss = true;
    init_success = true;
    pthread_setspecific(guard, (void *) 0);
}

void MemoryTracer::startMonitoringAllThreads() {
    LOGE("startMonitoringAllThreads");
    pthread_setspecific(guard, (void *) 1);
    cleanInfo();
    init_success = true;
    this->only_record_this_thread = false;
    this->enable_pss = true;
    this->enable_vss = true;
    pthread_setspecific(guard, (void *) 0);
}

void MemoryTracer::stopMonitoringThisThread() {
    LOGE("stopMonitoringThisThread");
    pthread_setspecific(guard, (void *) 1);
    init_success = false;
    this->only_record_this_thread = false;
    this->enable_pss = false;
    this->enable_vss = false;
    pthread_setspecific(guard, (void *) 0);
}

void MemoryTracer::stopAllMonitoring() {
    LOGE("stopAllMonitoring");
    pthread_setspecific(guard, (void *) 1);
    init_success = false;
    this->only_record_this_thread = false;
    this->enable_pss = false;
    this->enable_vss = false;
    pthread_setspecific(guard, (void *) 0);
}


bool MemoryTracer::saveLeakData(const char *path) {
    if (path == nullptr) {
        LOGE("file path is null");
        return false;
    }
    FILE *file = fopen(path, "w");
    if (file == nullptr) {
        LOGE("open %s file fail", path);
        return false;
    }
    void *dl_cache = nullptr;
    struct Args {
        FILE *file;
        void **dl_cache;
    };
    Args args = {file, &dl_cache};
    hashmapForEach(hashmap, hashCallbackFile, &args);
    xdl_addr_clean(&dl_cache);
    fclose(file);
    return true;
}

bool MemoryTracer::hashCallbackFile(void *key, void *value, void *context) {
    struct Args {
        FILE *file;
        void **dl_cache;
    };
    const char *tag = "DEBUG";
    Args *args = static_cast<struct Args *>(context);
    auto output = args->file;
    auto dl_cache = args->dl_cache;
    AllocNode *alloc_node = static_cast<AllocNode *>(value);

    fprintf(output, "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n");
    fprintf(output, "backtrace:\n");
    for (int i = 0; alloc_node->trace[i] != 0; i++) {
        uintptr_t pc = alloc_node->trace[i];
        xdl_info info;
        if (0 == xdl_addr((void *) pc, &info, dl_cache) || (uintptr_t) info.dli_fbase > pc) {
            fprintf(
                    output,
                    STACK_FORMAT_UNKNOWN,
                    i, pc
            );
        } else {
            if (nullptr == info.dli_fname || '\0' == info.dli_fname[0]) {
                fprintf(
                        output,
                        STACK_FORMAT_ANONYMOUS, i,
                        pc - (uintptr_t) info.dli_fbase,
                        (uintptr_t) info.dli_fbase
                );
            } else {
                if (nullptr == info.dli_sname || '\0' == info.dli_sname[0]) {
                    fprintf(
                            output,
                            STACK_FORMAT_FILE, i,
                            pc - (uintptr_t) info.dli_fbase,
                            info.dli_fname
                    );
                } else {
                    int s;
                    const char *symbol = __cxxabiv1::__cxa_demangle(
                            info.dli_sname,
                            nullptr,
                            nullptr,
                            &s
                    );
                    if (0 == (uintptr_t) info.dli_saddr || (uintptr_t) info.dli_saddr > pc) {
                        fprintf(
                                output,
                                STACK_FORMAT_FILE_NAME, i,
                                pc - (uintptr_t) info.dli_fbase,
                                info.dli_fname,
                                symbol == nullptr ? info.dli_sname : symbol
                        );
                    } else {
                        fprintf(
                                output,
                                STACK_FORMAT_FILE_NAME_LINE, i,
                                pc - (uintptr_t) info.dli_fbase,
                                info.dli_fname,
                                symbol == nullptr ? info.dli_sname : symbol,
                                pc - (uintptr_t) info.dli_saddr
                        );
                    }
                    if (symbol != nullptr) {
                        free((void *) symbol);
                    }
                }
            }
        }
    }

    return true;
}

bool MemoryTracer::dumpLeakInfo() {
    void *dl_cache = nullptr;
    hashmapForEach(hashmap, dumpLeakCallback, &dl_cache);
    xdl_addr_clean(&dl_cache);
    return false;
}

bool MemoryTracer::dumpLeakCallback(void *key, void *value, void *context) {
    const char *tag = "DEBUG";
    LOGDT("DEBUG", "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
    LOGDT(tag, "Build fingerprint: 'Android/sdk_phone64_x86_64/emulator64_x86_64:12/SC/7752726:userdebug/test-keys'");
    LOGDT("DEBUG", "backtrace:");

    void **dl_cache = static_cast<void **>(context);
    auto *alloc_node = static_cast<AllocNode *>(value);
    for (int i = 0; alloc_node->trace[i] != 0; i++) {
        uintptr_t pc = alloc_node->trace[i];
        xdl_info info;
        if (0 == xdl_addr((void *) pc, &info, dl_cache) || (uintptr_t) info.dli_fbase > pc) {
            LOGE(STACK_FORMAT_UNKNOWN, i, pc);
        } else {
            if (nullptr == info.dli_fname || '\0' == info.dli_fname[0]) {
                LOGDT(tag,
                      STACK_FORMAT_ANONYMOUS, i,
                      pc - (uintptr_t) info.dli_fbase,
                      (uintptr_t) info.dli_fbase
                );
            } else {
                if (nullptr == info.dli_sname || '\0' == info.dli_sname[0]) {
                    LOGDT(tag,

                          STACK_FORMAT_FILE, i,
                          pc - (uintptr_t) info.dli_fbase,
                          info.dli_fname
                    );
                } else {
                    int s;
                    const char *symbol = __cxxabiv1::__cxa_demangle(
                            info.dli_sname,
                            nullptr,
                            nullptr,
                            &s
                    );
                    if (0 == (uintptr_t) info.dli_saddr || (uintptr_t) info.dli_saddr > pc) {
                        LOGDT(tag,
                              STACK_FORMAT_FILE_NAME, i,
                              pc - (uintptr_t) info.dli_fbase,
                              info.dli_fname,
                              symbol == nullptr ? info.dli_sname : symbol
                        );
                    } else {
                        LOGDT(tag,
                              STACK_FORMAT_FILE_NAME_LINE, i,
                              pc - (uintptr_t) info.dli_fbase,
                              info.dli_fname,
                              symbol == nullptr ? info.dli_sname : symbol,
                              pc - (uintptr_t) info.dli_saddr
                        );
                    }
                    if (symbol != nullptr) {
                        free((void *) symbol);
                    }
                }
            }
        }
    }
    return true;

}

void MemoryTracer::cleanInfo() {
    if (hashmap == nullptr || hashmapSize(hashmap) <= 0) {
        return;
    }
    hashmapForEach(hashmap, dumpLeakCallback, nullptr);
    hashmapFree(hashmap);
    this->hashmap = hashmapCreate(1 << 15, hashmapUintPtrHash, hashmapUintPtrEquals);
}

bool MemoryTracer::cleanEntry(void *key, void *value, void *context) {
    real_free(value);
    return true;
}





