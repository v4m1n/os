#include "stdlib.h"
#include "stdio.h"

class Greeter {
public:
  void greet() {
    printf("Hello from userspace C++ application! %d, %s, %p\n", 42, "string", this);
  }
};

int main() {
  Greeter g;
  g.greet();
  return 0;
}
