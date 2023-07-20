#include <stdio.h>
#include <unistd.h>

static char* func2name(void* func) {
    return NULL;
}

void foo() {
    usleep(3000);
}

void bar() {
    usleep(2000);
}

int main(void) {
    foo();
    usleep(1000);
    bar();
    func2name(NULL);
    return 0;
}
