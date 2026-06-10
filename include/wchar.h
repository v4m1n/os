#ifndef _WCHAR_H
#define _WCHAR_H

#include <stddef.h>
#include <stdarg.h>

// Forward declarations of types that might be needed
struct _IO_FILE;
struct tm;

// wint_t is required by <cwchar>
#ifndef _WINT_T
#define _WINT_T
typedef unsigned int wint_t;
#endif

// mbstate_t is required by <cwchar>
#ifndef _MBSTATE_T
#define _MBSTATE_T
typedef struct {
    int __count;
    union {
        unsigned int __wch;
        char __wchb[4];
    } __value;
} mbstate_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// String/Memory functions
wchar_t* wcschr(const wchar_t* __p, wchar_t __c);
wchar_t* wcspbrk(const wchar_t* __s1, const wchar_t* __s2);
wchar_t* wcsrchr(const wchar_t* __p, wchar_t __c);
wchar_t* wcsstr(const wchar_t* __s1, const wchar_t* __s2);
wchar_t* wmemchr(const wchar_t* __p, wchar_t __c, size_t __n);

int wmemcmp(const wchar_t* __s1, const wchar_t* __s2, size_t __n);
size_t wcslen(const wchar_t* __s);
wchar_t* wmemmove(wchar_t* __s1, const wchar_t* __s2, size_t __n);
wchar_t* wmemcpy(wchar_t* __restrict __s1, const wchar_t* __restrict __s2, size_t __n);
wchar_t* wmemset(wchar_t* __s, wchar_t __c, size_t __n);

wchar_t* wcscat(wchar_t* __restrict __dest, const wchar_t* __restrict __src);
int wcscmp(const wchar_t* __s1, const wchar_t* __s2);
int wcscoll(const wchar_t* __s1, const wchar_t* __s2);
wchar_t* wcscpy(wchar_t* __restrict __dest, const wchar_t* __restrict __src);
size_t wcscspn(const wchar_t* __s1, const wchar_t* __s2);
wchar_t* wcsncat(wchar_t* __restrict __dest, const wchar_t* __restrict __src, size_t __n);
int wcsncmp(const wchar_t* __s1, const wchar_t* __s2, size_t __n);
wchar_t* wcsncpy(wchar_t* __restrict __dest, const wchar_t* __restrict __src, size_t __n);
size_t wcsspn(const wchar_t* __s1, const wchar_t* __s2);
wchar_t* wcstok(wchar_t* __restrict __s, const wchar_t* __restrict __delim, wchar_t** __restrict __ptr);
size_t wcsxfrm(wchar_t* __restrict __s1, const wchar_t* __restrict __s2, size_t __n);

// Conversion functions
float wcstof(const wchar_t* __restrict __nptr, wchar_t** __restrict __endptr);
double wcstod(const wchar_t* __restrict __nptr, wchar_t** __restrict __endptr);
long double wcstold(const wchar_t* __restrict __nptr, wchar_t** __restrict __endptr);
long wcstol(const wchar_t* __restrict __nptr, wchar_t** __restrict __endptr, int __base);
long long wcstoll(const wchar_t* __restrict __nptr, wchar_t** __restrict __endptr, int __base);
unsigned long wcstoul(const wchar_t* __restrict __nptr, wchar_t** __restrict __endptr, int __base);
unsigned long long wcstoull(const wchar_t* __restrict __nptr, wchar_t** __restrict __endptr, int __base);

// Input/Output / Scan functions
int fwide(struct _IO_FILE* __stream, int __mode);
int vfwscanf(struct _IO_FILE* __restrict __s, const wchar_t* __restrict __format, va_list __arg);
int vswscanf(const wchar_t* __restrict __s, const wchar_t* __restrict __format, va_list __arg);
int vwscanf(const wchar_t* __restrict __format, va_list __arg);

int fwscanf(struct _IO_FILE* __restrict __stream, const wchar_t* __restrict __format, ...);
int swscanf(const wchar_t* __restrict __s, const wchar_t* __restrict __format, ...);
int wscanf(const wchar_t* __restrict __format, ...);

int fputws(const wchar_t* __restrict __ws, struct _IO_FILE* __restrict __stream);
wint_t fputwc(wchar_t __c, struct _IO_FILE* __restrict __stream);
wint_t putwc(wchar_t __c, struct _IO_FILE* __restrict __stream);
wint_t putwchar(wchar_t __c);

wint_t fgetwc(struct _IO_FILE* __stream);
wint_t getwc(struct _IO_FILE* __stream);
wint_t getwchar(void);
wchar_t* fgetws(wchar_t* __restrict __ws, int __n, struct _IO_FILE* __restrict __stream);
wint_t ungetwc(wint_t __c, struct _IO_FILE* __stream);

int fwprintf(struct _IO_FILE* __restrict __stream, const wchar_t* __restrict __format, ...);
int wprintf(const wchar_t* __restrict __format, ...);
int swprintf(wchar_t* __restrict __s, size_t __n, const wchar_t* __restrict __format, ...);
int vfwprintf(struct _IO_FILE* __restrict __s, const wchar_t* __restrict __format, va_list __arg);
int vwprintf(const wchar_t* __restrict __format, va_list __arg);
int vswprintf(wchar_t* __restrict __s, size_t __n, const wchar_t* __restrict __format, va_list __arg);

size_t wcsftime(wchar_t* __restrict __s, size_t __maxsize, const wchar_t* __restrict __format, const struct tm* __restrict __tp);

// Character translation/collation
wint_t btowc(int __c);
int wctob(wint_t __c);
int mbsinit(const mbstate_t* __ps);
size_t mbrlen(const char* __restrict __s, size_t __n, mbstate_t* __restrict __ps);
size_t mbrtowc(wchar_t* __restrict __pwc, const char* __restrict __s, size_t __n, mbstate_t* __restrict __ps);
size_t wcrtomb(char* __restrict __s, wchar_t __wc, mbstate_t* __restrict __ps);
size_t mbsrtowcs(wchar_t* __restrict __dst, const char** __restrict __src, size_t __len, mbstate_t* __restrict __ps);
size_t wcsrtombs(char* __restrict __dst, const wchar_t** __restrict __src, size_t __len, mbstate_t* __restrict __ps);

#ifdef __cplusplus
}
#endif

#endif // _WCHAR_H
