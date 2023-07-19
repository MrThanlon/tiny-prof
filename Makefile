CC := gcc

all: test

run: test
	@./test

test: test.o hook.o
	$(CC) $^ -o $@ -ldl -rdynamic

test.o: test.c
	$(CC) -c $< -o $@ -finstrument-functions

hook.o: hook.c
	$(CC) -O3 -c $< -o $@

clean:
	-rm *.o test

.PHONY: all run