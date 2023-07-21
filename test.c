#include <stddef.h>
#include <stdio.h>
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

void* thread_task(void* data) {
    usleep(1000);
    bar();
    usleep(1000);
    foo();
    return NULL;
}

int main(void) {
    pthread_t t1;
    pthread_create(&t1, NULL, thread_task, NULL);
    foo();
    usleep(1000);
    bar();
    func2name(NULL);
    pthread_join(t1, NULL);
    return 0;
}
