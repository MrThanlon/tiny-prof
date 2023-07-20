CC := emcc

all: elfsym.js elfsym.wasm

elfsym.wasm elfsym.js: elfsym.c
	$(CC) $< -o elfsym.js -s WASM=1 -s EXPORTED_FUNCTIONS="['_malloc','_free']" -s EXPORT_ES6=1

clean:
	-rm elfsym.js elfsym.wasm

.PHONY: all
