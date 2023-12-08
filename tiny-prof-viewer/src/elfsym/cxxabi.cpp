// SPDX-License-Identifier: AGPL-3.0-or-later
#include <cxxabi.h>

extern "C" char* demangle(const char* mangled_name) {
    int status;
    char* demangled_name = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
    return demangled_name;
}
