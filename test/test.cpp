//
// Created by xiaobai on 2022/2/3.
//
#include <stdlib.h>
#include <cstring>
#include "../src/memory_tracer.h"
static int common_crash(bool status) {


    volatile int *p = reinterpret_cast<volatile int *>(0x100);
    p += 0x70000;
    p += *p + 0x70000;
    /* If it still doesnt crash..crash using null pointer */
    p = 0;
    p += *p;
    return *p;
}
void malloc_test() {
    void *buffer = malloc(10);
    LOGI("buffer %p", buffer);

    //common_crash(true);
}

void bar() {
    malloc_test();

}


void foo() {
    bar();

}

void malloc_free_test() {
    void *buffer = malloc(10);
    LOGI("buffer %p 1", buffer);
    free(buffer);
}

void bar1() {
    malloc_free_test();
}

void foo1() {
    bar1();
}


int main() {
    auto instance = MemoryTracer::getInstance();
    instance->startMonitoringThisThread();
    foo();
    foo1();
    instance->stopMonitoringThisThread();
    instance->dumpLeakInfo();
    instance->saveLeakData("/data/local/tmp/trace.log");
    return 0;
}

