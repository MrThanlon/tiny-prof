#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_LENGTH (1024 * 32)

struct call_info {
    uint64_t time_usec; // the highest bit represent enter or exit
    void* func;
    pthread_t pid;
} __attribute__((packed));

static struct call_info* call_records;
static uint32_t ptr = 0;

static uint64_t get_ticks()
{
    static volatile uint64_t time_elapsed = 0;
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

static uint64_t now(void) {
    return get_ticks() / 27;
}

void __attribute__((constructor)) traceBegin(void) {
    fprintf(stderr, "start profiling, build "__DATE__" "__TIME__"\n");
    fprintf(stderr, "traceBegin:%p\n", traceBegin);
    call_records = malloc(sizeof(struct call_info) * MAX_LENGTH);
}

void __attribute__((destructor)) traceEnd(void) {
    FILE* f = fopen("a.profile", "w");
    if (f) {
        // flag, indecate LE/BE
        uint32_t flag = 0x9982;
        fwrite(&flag, 1, 4, f);
        // pointer size
        uint32_t pointer_size = sizeof(void*);
        fwrite(&pointer_size, 1, 4, f);
        // the address of traceBegin
        size_t traceBeginAddress = (size_t)traceBegin;
        fwrite(&traceBeginAddress, 1, sizeof(void*), f);
        // records
        fwrite(call_records, 1, sizeof(struct call_info) * ptr, f);
        fclose(f);
        fprintf(stderr, "end profiling, %u records, checkout a.profile\n", ptr);
    } else {
        perror("end profiling, can't open file to save");
    }
    free(call_records);
}

void __cyg_profile_func_enter(void *func, void *caller) {
    if ((ptr >= MAX_LENGTH) || (call_records == NULL))
        return;
    struct call_info* info = &call_records[ptr++];
    info->func = func;
    info->pid = pthread_self();
    info->time_usec = (1ULL << 63ULL) | now();
}

void __cyg_profile_func_exit(void *func, void *caller) {
    if ((ptr >= MAX_LENGTH) || (call_records == NULL))
        return;
    struct call_info* info = &call_records[ptr++];
    info->func = func;
    info->pid = pthread_self();
    info->time_usec = now();
}
