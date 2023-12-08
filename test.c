// SPDX-License-Identifier: AGPL-3.0-or-later
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

static char* func2name(void* func) {
    return NULL;
}

void foo() {
    usleep(3000);
}

void bar() {
    usleep(2000);
}

__thread int th;

void thread_exit(void* arg) {
    printf("thread exit\n");
}

void* thread_task(void* data) {
    th = 114514;
    printf("thread th: %d\n", th);
    usleep(1000);
    bar();
    usleep(1000);
    foo();
    return NULL;
}

void* long_running_task(void* param) {
    fprintf(stderr, "%s: %lu\n", __func__, pthread_self());
    while (1) {
        foo();
    }
    return NULL;
}

int main(void) {
    th = 1919810;
    printf("main th: %d\n", th);
    pthread_t t1, t2;
    pthread_create(&t2, NULL, long_running_task, NULL);
    pthread_create(&t1, NULL, thread_task, NULL);
    foo();
    printf("main th: %d\n", th);
    usleep(1000);
    pthread_join(t1, NULL);
    bar();
    func2name(NULL);
    printf("main th: %d\n", th);
    sleep(1);
    pthread_cancel(t2);
    return 0;
}
