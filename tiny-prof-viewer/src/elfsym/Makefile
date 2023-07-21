CC := emcc

all: elfsym.js elfsym.wasm

elfsym.wasm elfsym.js: elfsym.c cxxabi.cpp
	$(CC) $^ -o elfsym.js -s WASM=1 -s EXPORTED_FUNCTIONS="['_malloc','_free']" -s EXPORT_ES6=1 -s INITIAL_MEMORY=67108864

clean:
	-rm elfsym.js elfsym.wasm

.PHONY: all
