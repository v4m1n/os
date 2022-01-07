#include "pci.h"
#include "stdint.h"
#include "asm.h"
#include "debug.h"
#include "nvme.h"

namespace pci {

  void deviceDetection() {
    dbg::printf("pci devices:\n");
    for (uint16_t bus = 0; bus < 256; ++bus) {
      for (uint8_t dev = 0; dev < 32; ++dev) {
        uint32_t vend = readPCIConfig<uint32_t>(bus, dev, 0, 0);
        if (vend == -1U) continue;
        uint16_t tmp1 = vend&0xffffU;
        uint16_t tmp2 = vend>>16;
        dbg::printf("  device {}:{}\n", tmp1, tmp2);
        auto cc = readPCIConfig<uint8_t>(bus, dev, 0, 0xb);
        auto sc = readPCIConfig<uint8_t>(bus, dev, 0, 0xa);
        auto prog_if = readPCIConfig<uint8_t>(bus, dev, 0, 9);
        if (cc == 1 && sc == 8 && prog_if == 2) {
          dbg::printf("    NVME\n");
          NVMe drive(bus, dev);
        }

      }
    }
  }
  //void deviceDetection() {
  //  uint8_t max = 1;
  //  uint8_t cont_type = readPCIConfig<uint8_t>(0, 0, 0, 0xe);
  //  dbg::panic_assert(cont_type != 0xffU, "pci controller not available\n");
  //  if (cont_type & 0x80) {
  //    max = 8;
  //  }
  //  dbg::printf("pci devices:\n");
  //  for (uint8_t func = 0; func < max; ++func) {
  //    if (readPCIConfig<uint32_t>(0, 0, func, 0) == -1U) break;
  //    for (uint8_t dev = 0; dev < 32; ++dev) {
  //      uint32_t vend = readPCIConfig<uint32_t>(func, dev, 0, 0);
  //      if (vend == -1U) continue;
  //      uint16_t tmp1 = vend&0xffffU;
  //      uint16_t tmp2 = vend>>16;
  //      dbg::printf("  device {}:{}\n", tmp1, tmp2);
  //    }
  //  }
  //}

}
