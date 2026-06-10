module;
#include "stdint.h"
#include "stddef.h"
#include "asm.h"

module ahci;
import utility;
import debug;
import vmm;
import pmm;
import pci;
import interrupts;


AHCI::AHCI(uint8_t bus, uint8_t dev) : bus_(bus), dev_(dev) {
  uint16_t pcicmd = pci::readPCIConfig<uint16_t>(bus, dev, 0, 0x4);
  pci::writePCIConfig<uint16_t>(bus, dev, 0, 0x4, (pcicmd & ~(1U<<10)) | 1U<<1 | 1U<<2 | 1U | 1U<<4);
  //uint64_t bar1 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x10);
  uint64_t bar5 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x24);
  //dbg::panic_assert(bar1 != -1U && bar2 != -1U, "nvme device does not exist ({}, {})\n", bus, dev);
  //bar_phys_ = (bar1 & ~0xfU) | bar2<<32;
  //dbg::panic_assert(bar_phys_&~(PAGE_SIZE-1), "nvme device mmio not page aligned {}\n", bar_phys_);
  //dbg::printf("nvme mmio {}\n", bar_phys_);
  hba_ = (volatile HBA_MEM *)vmm::AddressSpace::ioremap(bar5/PAGE_SIZE);
  //doorbells_ = vmm::identUCAddress<uint16_t *>(bar_phys_+0x1000);
  dbg::printf("ghc: {}\n", hba_->ghc);
  dbg::printf("bios/os handoff: {}\n", hba_->bohc);
  if (hba_->bohc & 1) { 
    hba_->bohc |= 2;
    while (!(hba_->bohc&2)) asm volatile("pause" ::: "memory");
  }
  dbg::printf("reseting AHCI device...\n");
  hba_->ghc |= 1; // device reset;
  while (hba_->ghc&1) asm volatile("pause" ::: "memory");

  uint8_t irq = pci::readPCIConfig<uint8_t>(bus, dev, 0, 0x3c);
  dbg::printf("ahci irq line {}\n", irq);

  hba_->ghc |= 1U<<31 | 1U<<1;
  //FIS_REG_H2D fis{};
  //fis.fis_type = FIS_TYPE_REG_H2D;
  //fis.command = 0xEC;
  //fis.device = 0;      // Master device
  //fis.c = 1; 
  dbg::panic_assert(hba_->ghc>>31, "AHCI not enabled ({})\n", hba_->ghc);


  for (size_t i = 0; i < 1; ++i) {
    size_t page = pmm::allocZeroPFN()*PAGE_SIZE;
    hba_->ports[i].clb = page;
    hba_->ports[i].clbu = page>>32;
    cmd_buffers[i] = vmm::identAddress<volatile FIS *>(page);

    page = pmm::allocZeroPFN()*PAGE_SIZE;
    hba_->ports[i].fb = page;
    hba_->ports[i].fbu = page>>32;
    fis_buffers[i] = vmm::identAddress<volatile FIS *>(page);
  }

  //dbg::panic("done\n");
}





