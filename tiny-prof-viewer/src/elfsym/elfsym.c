#include <stddef.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

char* demangle(const char* mangled_name);

struct malloc_buffer {
    size_t size;
    size_t capacity;
    char* buffer;
};

static void malloc_buffer_append(struct malloc_buffer* buffer, const void* appended, size_t size) {
    if (buffer->capacity < buffer->size + size) {
        // expand
        buffer->capacity += size + 1024;
        buffer->buffer = realloc(buffer->buffer, buffer->capacity);
    }
    memcpy(buffer->buffer + buffer->size, appended, size);
    buffer->size += size;
}

static void malloc_buffer_init(struct malloc_buffer* buffer) {
    buffer->size = 0;
    buffer->capacity = 1024;
    buffer->buffer = malloc(buffer->capacity);
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void* elfsym_load(const void* buffer, size_t size) {
    const Elf64_Ehdr *ehdr = buffer;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "It's not an ELF file, %d%d%d\n", ehdr->e_ident[1], ehdr->e_ident[2], ehdr->e_ident[3]);
        return NULL;
    }

    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)buffer + ehdr->e_shoff);
    struct malloc_buffer output_buf;
    malloc_buffer_init(&output_buf);
    // first 8bit for num of symbols
    output_buf.size += 8;
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            #ifdef __EMSCRIPTEN__
            printf("Found a .symtab at offset 0x%llx\n", shdr[i].sh_offset);
            #else
            printf("Found a .symtab at offset 0x%lx\n", shdr[i].sh_offset);
            #endif

            Elf64_Sym *sym = (Elf64_Sym *)((char *)buffer + shdr[i].sh_offset);
            for (int j = 0; j < shdr[i].sh_size / sizeof(Elf64_Sym); j++) {
                if (ELF64_ST_TYPE(sym[j].st_info) != STT_FUNC && ELF64_ST_TYPE(sym[j].st_info) != STT_FILE) {
                    continue;
                }
                if (sym[j].st_value == 0) {
                    continue;
                }
                //append((char *)buffer + shdr[shdr[i].sh_link].sh_offset + sym[j].st_name, sym[j].st_value);
                // address
                uint64_t addr = sym[j].st_value;
                malloc_buffer_append(&output_buf, &addr, sizeof(uint64_t));

                // name string
                char* name = (char *)buffer + shdr[shdr[i].sh_link].sh_offset + sym[j].st_name;
                // demangle
                char* demangled_name = demangle(name);
                if (demangled_name) {
                    uint8_t name_len = strlen(demangled_name);
                    malloc_buffer_append(&output_buf, &name_len, 1);
                    malloc_buffer_append(&output_buf, demangled_name, name_len);
                    free(demangled_name);
                } else {
                    uint8_t name_len = strlen(name);
                    malloc_buffer_append(&output_buf, &name_len, 1);
                    malloc_buffer_append(&output_buf, name, name_len);
                }
                if (strcmp(name, "traceBegin") == 0)
                #ifdef __EMSCRIPTEN__
                printf("%08llx: %s\n",
                #else
                printf("%08lx: %s\n",
                #endif
                    sym[j].st_value,
                    name
                );
            }
            uint64_t total_symbols = shdr[i].sh_size / sizeof(Elf64_Sym);
            #if __WORDSIZE == 64
            printf("total symbols: %lu\n", total_symbols);
            #else
            printf("total symbols: %llu\n", total_symbols);
            #endif
            memcpy(output_buf.buffer, &total_symbols, 8);
            uint64_t end = 0;
            malloc_buffer_append(&output_buf, &end, sizeof(end));
            break;
        }
    }
    return output_buf.buffer;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void elfsym_test() {
    printf("Load wasm success\n");
}

#ifndef __EMSCRIPTEN__
int main(int argc, char* argv[]) {
    FILE* f = fopen(argv[1], "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        void* buffer = malloc(size);
        fseek(f, 0, SEEK_SET);
        fread(buffer, 1, size, f);
        free(elfsym_load(buffer, size));
        fclose(f);
    }
    return 0;
}
#endif
