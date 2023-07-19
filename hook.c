#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static struct call_info* call_records;
static uint32_t ptr = 0;

static uint64_t now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void __attribute__((constructor)) traceBegin(void) {
    fprintf(stderr, "start profiling, build "__DATE__" "__TIME__"\n");
    call_records = malloc(sizeof(struct call_info) * MAX_LENGTH);
}

void __attribute__((destructor)) traceEnd(void) {
    FILE* f = fopen("a.profile", "w");
    if (f) {
        // name-addr table
        uint32_t tb_len = 0;
        long data_begin_pos = 0;
        fseek(f, 4, SEEK_SET);
        for (uint32_t i = 0; i < ptr; i++) {
            for (uint32_t j = 0; j < i; j++) {
                if (call_records[i].func == call_records[j].func) {
                    // exist, ignore
                    goto ignore;
                }
            }
            // add a enity
            fwrite(&call_records[i].func, 1, sizeof(void*), f);
            const char* name = func2name(call_records[i].func);
            fwrite(name, 1, strlen(name) + 1, f);
            tb_len += 1;
            ignore:
            continue;
        }
        data_begin_pos = ftell(f);
        fseek(f, 0, SEEK_SET);
        fwrite(&tb_len, 1, 4, f);
        fseek(f, data_begin_pos, SEEK_SET);
        // records
        fwrite(call_records, 1, sizeof(struct call_info) * ptr, f);
        fclose(f);
        fprintf(stderr, "end profiling, checkout a.profile\n");
    } else {
        perror("end profiling, can't open file to save");
    }
    free(call_records);
}

void __cyg_profile_func_enter(void *func, void *caller) {
    //fprintf(stderr, "enter func: %s father: %s\n", func2name(func), func2name(caller));
    struct call_info* info = &call_records[ptr++];
    info->func = func;
    info->time_usec = (1ULL << 63) & now();
}

void __cyg_profile_func_exit(void *func, void *caller) {
    //fprintf(stderr, "exit func: %s father: %s\n", func2name(func), func2name(caller));
    struct call_info* info = &call_records[ptr++];
    info->func = func;
    info->time_usec = now();
}
