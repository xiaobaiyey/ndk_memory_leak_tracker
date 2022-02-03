//
// Created by xiaobai on 2022/2/3.
//
#include <stdlib.h>
#include <cstring>
#include "../src/memory_tracer.h"

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

