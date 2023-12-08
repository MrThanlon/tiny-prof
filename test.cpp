// SPDX-License-Identifier: AGPL-3.0-or-later
#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;

static void foo() {
    this_thread::sleep_for(chrono::milliseconds(1));
}

static void bar() {
    this_thread::sleep_for(chrono::milliseconds(2));
}

static void t1_proc() {
    this_thread::sleep_for(chrono::milliseconds(5));
    bar();
    this_thread::sleep_for(chrono::milliseconds(3));
    foo();
}

int main(void) {
    thread t1(t1_proc);
    foo();
    this_thread::sleep_for(chrono::microseconds(500));
    bar();
    t1.join();
    return 0;
}
