module;
#include <cstdint>
#include <cstddef>

export module string;

extern "C" {
export void *memcpy(void *dest, const void *src, size_t n);
export int memcmp(const void *s1, const void *s2, size_t n);
export void *memset(void *s, int c, size_t n);
export int strcmp(const char *s1, const char *s2);
export int strncmp(const char *s1, const char *s2, size_t n);
export char *strncpy(char *dest, const char *src, size_t n);
export size_t strlen(const char *s);
}
