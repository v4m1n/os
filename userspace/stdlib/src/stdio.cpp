#include "stdio.h"
#include "syscall.h"
#include <stdarg.h>

extern "C" void print(const char* str) {
  if (!str) return;
  size_t len = 0;
  while (str[len] != '\0') {
    len++;
  }
  asm volatile(
    "mov $1, %%rax; int $0x80"
    :
    : "S"((uint64_t)str), "c"((uint64_t)len)
    : "rax"
  );
}

static void print_char(char c) {
  char buf[2] = {c, 0};
  print(buf);
}

static void print_string(const char* s) {
  if (!s) s = "(null)";
  print(s);
}

static void print_unsigned(unsigned long long n, int base) {
  char buf[32];
  int i = 0;
  if (n == 0) {
    print_char('0');
    return;
  }
  while (n > 0) {
    int digit = n % base;
    buf[i++] = (digit < 10) ? ('0' + digit) : ('a' + (digit - 10));
    n /= base;
  }
  for (int j = i - 1; j >= 0; --j) {
    print_char(buf[j]);
  }
}

static void print_signed(long long n, int base) {
  if (n < 0) {
    print_char('-');
    print_unsigned(-n, base);
  } else {
    print_unsigned(n, base);
  }
}

extern "C" void printf(const char* format, ...) {
  va_list args;
  va_start(args, format);

  for (const char* p = format; *p != '\0'; ++p) {
    if (*p != '%') {
      print_char(*p);
      continue;
    }

    ++p; // skip '%'
    if (*p == '\0') {
      print_char('%');
      break;
    }

    switch (*p) {
      case '%':
        print_char('%');
        break;
      case 's': {
        const char* s = va_arg(args, const char*);
        print_string(s);
        break;
      }
      case 'c': {
        char c = (char)va_arg(args, int);
        print_char(c);
        break;
      }
      case 'd':
      case 'i': {
        int d = va_arg(args, int);
        print_signed(d, 10);
        break;
      }
      case 'u': {
        unsigned int u = va_arg(args, unsigned int);
        print_unsigned(u, 10);
        break;
      }
      case 'x':
      case 'X': {
        unsigned int x = va_arg(args, unsigned int);
        print_unsigned(x, 16);
        break;
      }
      case 'p': {
        void* ptr = va_arg(args, void*);
        print_string("0x");
        print_unsigned((unsigned long long)ptr, 16);
        break;
      }
      default:
        print_char('%');
        print_char(*p);
        break;
    }
  }

  va_end(args);
}
