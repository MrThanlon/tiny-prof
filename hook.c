#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_LENGTH (1024 * 32)

#define DEBUG(...)
#define INFO(...) fprintf(stderr,##__VA_ARGS__)

static uint64_t now(void) {
#ifdef __riscv
    static volatile uint64_t time_elapsed = 0;
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed / 27;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
#endif
}

static FILE* f;
// TODO: optimize multi-thread to different buffer
static pthread_mutex_t write_file_mutex;

void __attribute__((constructor)) traceBegin(void) {
    INFO("start profiling, build " __DATE__ " " __TIME__ "\n");
    INFO("traceBegin:%p\n", traceBegin);
    f = fopen("a.profile", "w");
    if (f) {
        pthread_mutex_init(&write_file_mutex, NULL);
        // TODO: multi-thread
        // flag, indecate LE/BE
        uint32_t flag = 0x9982;
        fwrite(&flag, 1, 4, f);
        // pointer size
        struct {
            uint32_t pointer_size: 4;
            uint32_t pthread_size: 4;
        } sizes;
        sizes.pointer_size = sizeof(void*);
        sizes.pthread_size = sizeof(pthread_t);
        fwrite(&sizes, 1, 4, f);
        // the address of traceBegin
        size_t traceBeginAddress = (size_t)traceBegin;
        fwrite(&traceBeginAddress, 1, sizeof(void*), f);
    } else {
        INFO("open a.profile error, profile will not be saved\n");
    }
}

static uint32_t total_records = 0;

void __attribute__((destructor)) traceEnd(void) {
    if (f) {
        pthread_mutex_destroy(&write_file_mutex);
        // records
        fclose(f);
        INFO("end profiling, %u records, checkout a.profile\n", total_records);
    }
}

struct call_info {
    uint64_t time_usec; // the highest bit represent enter or exit
    void* func;
} __attribute__((packed));

// record to it's own data struct
#define RECORD_LENGTH (1024 * 32)
struct thread_record_block {
    pthread_t tid;
    uint64_t records_size;
    struct call_info records[];
}__attribute__((packed));
static __thread struct thread_record_block * trb = NULL;
static __thread unsigned stacks = 0;

static void write_file(void) {
    struct call_info wr_records[2] = {
        {
            .func = write_file,
            .time_usec = (1UL << 63UL) | now()
        }, {
            .func = write_file,
            .time_usec = 0
        }
    };
    trb->records_size += 2;
    pthread_mutex_lock(&write_file_mutex);
    fwrite(trb, 1, sizeof(struct thread_record_block) + (trb->records_size - 2) * sizeof(struct call_info), f);
    wr_records[1].time_usec = now();
    fwrite(wr_records, 1, 2 * sizeof(struct call_info), f);
    pthread_mutex_unlock(&write_file_mutex);
    DEBUG("write record of thread %lu\n", trb->tid);
    trb->records_size = 0;
}

void __cyg_profile_func_enter(void *func, void *caller) {
    if (!f) {
        return;
    }
    if (trb == NULL) {
        trb = malloc(sizeof(struct thread_record_block) + RECORD_LENGTH * sizeof(struct call_info));
        trb->tid = pthread_self();
        stacks = 0;
        trb->records_size = 0;
    }
    DEBUG("thread %lu enter %p\n", trb->tid, func);
    if (trb->records_size >= RECORD_LENGTH) {
        // save to file
        write_file();
    }
    struct call_info* record = &trb->records[trb->records_size++];
    record->func = func;
    record->time_usec = (1UL << 63UL) | now();
    stacks += 1;
    __sync_fetch_and_add(&total_records, 1);
}

void __cyg_profile_func_exit(void *func, void *caller) {
    if ((!f) || (trb == NULL) || (stacks == 0)) {
        return;
    }
    if (trb->records_size >= RECORD_LENGTH) {
        // save to file
        write_file();
    }
    DEBUG("thread %lu exit %p, stacks: %u\n", trb->tid, func, stacks);
    struct call_info* record = &trb->records[trb->records_size++];
    record->func = func;
    record->time_usec = now();
    stacks -= 1;
    if (stacks == 0) {
        // thread end, write to file
        write_file();
    }
    __sync_fetch_and_add(&total_records, 1);
}
