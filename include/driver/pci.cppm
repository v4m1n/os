module;
#include "stdint.h"
#include "asm.h"

export module pci;

export namespace pci {

  constexpr uint16_t PCI_CONFIG_ADDRESS = 0xcf8U;
  constexpr uint16_t PCI_CONFIG_DATA = 0xcfcU;
  
  template<typename T>
  T readPCIConfig(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset) {
    uint32_t address = bus<<16 | slot<<11 | func<<8 | (offset & ~3U) | 1U<<31;
    out(PCI_CONFIG_ADDRESS, address);
    if constexpr (sizeof(T) == 1)
      return inb(PCI_CONFIG_DATA + (offset & 3));
    else if constexpr (sizeof(T) == 2)
      return inw(PCI_CONFIG_DATA + (offset & 3));
    else if constexpr (sizeof(T) == 4)
      return in(PCI_CONFIG_DATA);

    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "invalid read size");
  }
  
  template<typename T>
  void writePCIConfig(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, T value) {
    uint32_t address = bus<<16 | slot<<11 | func<<8 | (offset & ~3U) | 1U<<31;
    out(PCI_CONFIG_ADDRESS, address);
    if constexpr (sizeof(T) == 1)
      outb(PCI_CONFIG_DATA + (offset & 3), value);
    else if constexpr (sizeof(T) == 2)
      outw(PCI_CONFIG_DATA + (offset & 3), value);
    else if constexpr (sizeof(T) == 4)
      out(PCI_CONFIG_DATA, value);

    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "invalid read size");
  }
  
  void deviceDetection();

}
