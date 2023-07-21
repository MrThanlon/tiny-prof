CC := gcc
CXX := g++

all: test test_cpp

run: test test_cpp
	@./test
	@./test_cpp

test: test.o hook.o
	$(CC) -g $^ -o $@

test_cpp: test_cpp.o hook.o
	$(CXX) -g $^ -o $@

test.o: test.c
	$(CC) -Og -g -c $< -o $@ -finstrument-functions

test_cpp.o: test_cpp.cpp
	$(CXX) -Og -g -c $< -o $@ -finstrument-functions

hook.o: hook.c
	$(CC) -O3 -c $< -o $@

clean:
	-rm *.o test test_cpp

.PHONY: all run
