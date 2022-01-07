#pragma once
#include "stdint.h"
#include "asm.h"

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
  void deviceDetection();

}
