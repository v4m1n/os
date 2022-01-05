#pragma once
#include "stdint.h"
#include "stddef.h"
#include "asm.h"
#include "sync.h"

namespace dbg {

  void dump(void *data, size_t size);

  void printfu(const char *str);

  void putchar(const char x);

  extern spinlock_irq lock;

  template<typename T>
  void printdec(T arg) {
    static_assert(sizeof(T) <= 8);
    char buf[21];
    buf[20] = 0;
    size_t i = 20;

    do {
      buf[--i] = '0'+(arg%10);
      arg /= 10;
    } while(arg);
    printfu(buf+i);
  }
  template<typename T>
  void printhex(T arg) {
    static_assert(sizeof(T) <= 8);
    putchar('0');
    putchar('x');
    for (size_t i = sizeof(T)*2; i; --i) {
      char x = (arg>>((i-1)*4))&0xf;
      if (x >= 10) {
        putchar('a'+(x-10));
      }
      else {
        putchar('0'+x);
      }
    }
  }
  
  template<typename, typename>
  struct is_same_type {
    const static bool value = false;
  };
  template<typename T>
  struct is_same_type<T,T> {
    const static bool value = true;
  };

  template<typename T>
  void printhex(T *arg) {
    printhex((uint64_t)arg);
  }
  
  template<typename T>
  void printdec(T *arg) {
    printdec((uint64_t)arg);
  }
  
  template<typename T, typename ...U>
  void printfu(const char *str, const T arg, const U... args) {
    while(*str) {
      if (*str == '{') {
        if (!*++str) {
          putchar('{');
          return;
        }
        char x = *str;

        if (x == '}') {
          if constexpr(is_same_type<T, char *>::value ||
                       is_same_type<T, const char *>::value ||
                       is_same_type<T, char * const>::value ||
                       is_same_type<T, const char * const>::value) {
            printfu(arg);
          }
          else {
            printhex(arg);
          }
          return printfu(++str, args...);
        }
        if (*++str != '}') {
          putchar('{');
          putchar(x);
          goto next;
        }
        if (x == 'x') {
          printhex(arg);
          return printfu(++str, args...);
        }
        if (x == 'd') {
          printdec(arg);
          return printfu(++str, args...);
        }
        putchar('{');
        putchar(x);
      }
  next:
      putchar(*str);
      ++str;
    }
  }
  template<typename ...U>
  void printf(const char *str, const U... args) {
    lock_guard lck(lock);
    printfu(str, args...);
  }

  template<typename ...U>
  [[noreturn]] void panic(const char *str, const U... args) {
    printf("==========KERNEL PANIC==========\n");
    printf(str, args...);
    while(1) hlt();
  }

  template<typename ...U>
  void panic_assert(bool cond, const char *str, const U... args) {
    if (!cond) {
      panic(str, args...);
    }
  }
}
