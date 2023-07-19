#include <stdio.h>
#include <unistd.h>

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
    return 0;
}
