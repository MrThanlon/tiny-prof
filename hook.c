#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>

#define MAX_LENGTH (1024 * 32)

static const char* func2name(void* func) {
    Dl_info info;
    dladdr(func, &info);
    return info.dli_sname;
}

struct call_info {
    void* func;
    uint64_t time_usec; // the highest bit represent enter or exit
};

static struct call_info* call_chain;
static unsigned ptr = 0;

static uint64_t now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void __attribute__((constructor)) traceBegin(void) {
    fprintf(stderr, "start profiling, build "__DATE__" "__TIME__"\n");
    call_chain = malloc(sizeof(struct call_info) * MAX_LENGTH);
}

void __attribute__((destructor)) traceEnd(void) {
    // read function name
    FILE* f = fopen("a.profile", "w");
    if (f) {
        fwrite(call_chain, 1, sizeof(struct call_info) * ptr, f);
        fclose(f);
        fprintf(stderr, "end profiling, checkout a.profile\n");
    } else {
        perror("end profiling, can't open file to save");
    }
    free(call_chain);
}

void __cyg_profile_func_enter(void *func, void *caller) {
    //fprintf(stderr, "enter func: %s father: %s\n", func2name(func), func2name(caller));
    struct call_info* info = &call_chain[ptr++];
    info->func = func;
    info->time_usec = (1ULL << 63) & now();
}

void __cyg_profile_func_exit(void *func, void *caller) {
    //fprintf(stderr, "exit func: %s father: %s\n", func2name(func), func2name(caller));
    struct call_info* info = &call_chain[ptr++];
    info->func = func;
    info->time_usec = now();
}
