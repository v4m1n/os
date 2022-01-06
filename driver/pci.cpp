#include "pci.h"
#include "stdint.h"
#include "asm.h"
#include "debug.h"

namespace pci {
  constexpr uint16_t PCI_CONFIG_ADDRESS = 0xcf8U;
  constexpr uint16_t PCI_CONFIG_DATA = 0xcfcU;

  template<typename T>
  T readPCIConfig(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset) {
    uint32_t address = bus<<16 | slot<<11 | func<<8 | offset | 1U<<31;
    out(PCI_CONFIG_ADDRESS, address);
    if constexpr (sizeof(T) == 1)
      return inb(PCI_CONFIG_DATA);
    else if constexpr (sizeof(T) == 2)
      return inw(PCI_CONFIG_DATA);
    else if constexpr (sizeof(T) == 4)
      return in(PCI_CONFIG_DATA);

    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "invalid read size");
  }

  void deviceDetection() {
    uint32_t max = 1;
    uint8_t cont_type = readPCIConfig<uint8_t>(0, 0, 0, 0xe);
    dbg::panic_assert(cont_type != 0xffU, "pci controller not available\n");
    if (cont_type & 0x80) {
      max = 8;
    }
    dbg::printf("pci devices:\n");
    for (uint32_t func = 0; func < max; ++func) {
      if (readPCIConfig<uint32_t>(0, 0, func, 0) == -1U) break;
      for (uint32_t dev = 0; dev < 32; ++dev) {
        uint32_t vend = readPCIConfig<uint32_t>(func, dev, 0, 0);
        if (vend == -1U) continue;
        uint16_t tmp1 = vend&0xffffU;
        uint16_t tmp2 = vend>>16;
        dbg::printf("  device {}:{}\n", tmp1, tmp2);
      }
    }
  }

}
