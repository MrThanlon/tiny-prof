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

int main(void) {
    th = 1919810;
    printf("main th: %d\n", th);
    pthread_t t1;
    pthread_create(&t1, NULL, thread_task, NULL);
    foo();
    printf("main th: %d\n", th);
    usleep(1000);
    pthread_join(t1, NULL);
    bar();
    func2name(NULL);
    printf("main th: %d\n", th);
    return 0;
}
