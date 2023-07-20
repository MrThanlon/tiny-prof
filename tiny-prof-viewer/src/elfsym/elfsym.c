#include <stddef.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
    unsigned long* output_buffer = malloc(1024);
    size_t output_buffer_idx = 0;
    size_t output_buffer_cap = 1024;
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
                // append
                if (output_buffer_idx + 1 >= output_buffer_cap) {
                    output_buffer = realloc(output_buffer, output_buffer_cap + 1024);
                    output_buffer_cap += 1024;
                }
                output_buffer[output_buffer_idx++] = sym[j].st_value;
                char* name = (char *)buffer + shdr[shdr[i].sh_link].sh_offset + sym[j].st_name;
                if (output_buffer_idx + strlen(name) >= output_buffer_cap) {
                    output_buffer = realloc(output_buffer, output_buffer_cap + 1024);
                    output_buffer_cap += 1024;
                }
                memcpy(output_buffer + output_buffer_idx, name, strlen(name) + 1);
                output_buffer_idx += strlen(name) + 1;
                #ifdef __EMSCRIPTEN__
                printf("%08llx: %s\n",
                #else
                printf("%08lx: %s\n",
                #endif
                    sym[j].st_value,
                    name
                );
            }
        }
    }
    return output_buffer;
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
