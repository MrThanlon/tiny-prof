#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

static void foo() {
    this_thread::sleep_for(chrono::milliseconds(1));
}

static void bar() {
    this_thread::sleep_for(chrono::milliseconds(2));
}

int main(void) {
    foo();
    this_thread::sleep_for(chrono::microseconds(500));
    bar();
    return 0;
}