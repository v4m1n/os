module;
#include <cstdint>

export module multiboot;
import acpi;

export namespace mboot {
  void parse(uint64_t header);
  extern RSDP *rsdp;
}
