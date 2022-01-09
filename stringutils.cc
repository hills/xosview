#include "stringutils.h"

#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void snprintf_or_abort(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int needed = vsnprintf(str, size, format, args);
    va_end(args);
    if (needed < 0) {
        std::cerr << "Internal error: vsnprintf returned an error in "
                "snprintf_or_abort" << std::endl;
        abort();
    }
    if ((size_t)needed >= size) {
      std::cerr << "Internal error: Buffer to snprintf_or_abort too small, "
          "required " << needed << " but got " << size << std::endl;
      abort();
    }
}
