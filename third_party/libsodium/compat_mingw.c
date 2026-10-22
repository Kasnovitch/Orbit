/* compat for mingw-w64 runtimes older than the libsodium static build:
   memset_explicit is C23 and not in every mingw-w64 CRT. Same trick as
   libmingwex: real memset plus a compiler barrier so it can't be elided. */
#include <stddef.h>
#include <string.h>

void *memset_explicit(void *dest, int ch, size_t count);

void *memset_explicit(void *dest, int ch, size_t count)
{
    void *result = memset(dest, ch, count);
    __asm__ __volatile__("" : : "r"(dest) : "memory");
    return result;
}
