#include "stdint.h"
#include "debug.h"
#include "utility.h"
#include "gdt.h"
#include "asm.h"
#include "vmm.h"
#include "pmm.h"
#include "mpt.h"
#include "acpi.h"
#include "string.h"
#include "interrupts.h"
#include "scheduler.h"

#define INTERRUPT_GATE 0xE
#define TRAP_GATE 0xF

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

struct IntDes {
  IntDes(uint64_t offset, uint16_t segment, 
         uint8_t type, uint8_t dpl, uint8_t present) :
    offset1_(offset&0xffffU), segment_(segment), 
    gate_type_(type), zero_(0), dpl_(dpl),
    present_(present), offset2_((offset>>16)&0xffffU),
    offset3_(offset>>32) {}
  consteval IntDes() = default;
  uint16_t offset1_ = 0;
  uint16_t segment_ = 0;
  uint8_t reserved1_ = 0;
  uint8_t gate_type_ : 4 = 0;
  uint8_t zero_ : 1 = 0;
  uint8_t dpl_ : 2 = 0;
  uint8_t present_ : 1 = 0;
  uint16_t offset2_ = 0;
  uint32_t offset3_ = 0;
  uint32_t reserved2_ = 0;
} __attribute__((packed));

static_assert(sizeof(IntDes) == 16);

extern "C" void asm_exception_handler_0();
extern "C" void asm_exception_handler_8();
extern "C" void asm_exception_handler_13();
extern "C" void asm_exception_handler_14();
extern "C" void asm_irq_handler_48();
extern "C" void asm_irq_handler_128();
extern "C" void asm_irq_handler_254();
extern "C" void asm_irq_handler_255();
extern "C" void asm_irq_handler_def();

pair<uint8_t, void (*)()> irq_handlers[] = {
  {0, asm_exception_handler_0},
  {8, asm_exception_handler_8},
  {13, asm_exception_handler_13},
  {14, asm_exception_handler_14},
  {48, asm_irq_handler_48},
  {128, asm_irq_handler_128},
  {254, asm_irq_handler_254},
  {255, asm_irq_handler_255},
};

irq::APIC *apic;

IntDes idt[256];

extern "C"
void exception_handler_0() {
}
extern "C"
void exception_handler_8() {
  dbg::printf("Double Fault\n");
}
extern "C"
void exception_handler_13() {
  dbg::panic("General Protection Fault\n");
}
extern "C"
void exception_handler_14() {
  dbg::panic("pf\n");
}

extern "C"
void irq_handler_48() {
  apic->EOI = 0;
  sched::schedule();
}
extern "C"
void irq_handler_128() {
  dbg::printf("hello\n");
}
extern "C"
void irq_handler_254() {
  dbg::panic("APIC error\n");
}
extern "C"
void irq_handler_255() {
  dbg::printf("spurious int\n");
}
extern "C"
void irq_handler_def() {
  dbg::printf("unknown int\n");
}

extern size_t VIRTUAL_OFFSET;

namespace irq {

void initIdt() {
  dbg::printf("initializing IDT...\n");

  uint16_t max_num = 0;
  for (auto [num, func] : irq_handlers) {
    idt[num] = IntDes(reinterpret_cast<uint64_t>(func), KERNEL_CS, INTERRUPT_GATE, 0, 1);
    
    if (num > max_num)
      max_num = num;
  }
  for (auto &el : idt) {
    if (el.present_)
      continue;
    //el = IntDes(reinterpret_cast<uint64_t>(asm_irq_handler_def), KERNEL_CS, INTERRUPT_GATE, 0, 1);
  }

  struct lidt {
    uint16_t size_;
    uint64_t addr_;
  } __attribute__((packed));
    
  static_assert(sizeof(lidt) == 10);
  lidt tmp{static_cast<uint16_t>(sizeof(idt)-1), reinterpret_cast<uint64_t>(idt)};

  asm volatile("lidt %0"::"m"(tmp):"memory");
  //asm volatile("int 0x80");
}

#define PIC1_COMMAND 0x20
#define PIC2_COMMAND 0xA0
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

#define APIC_MASK (1U<<16)

static void remapDisablePIC() {
  outb(PIC1_COMMAND, 0x10|0x01);
  outb(0x80, 0);
  outb(PIC2_COMMAND, 0x10|0x01);
  outb(0x80, 0);
  outb(PIC1_DATA, 0x20);
  outb(0x80, 0);
  outb(PIC2_DATA, 0x28);
  outb(0x80, 0);
  outb(PIC1_DATA, 4);
  outb(0x80, 0);
  outb(PIC2_DATA, 2);
  outb(0x80, 0);
  outb(PIC1_DATA, 1);
  outb(0x80, 0);
  outb(PIC2_DATA, 1);
  outb(0x80, 0);
  outb(PIC1_DATA, 0xffU);
  outb(PIC2_DATA, 0xffU);
}

void initAPIC() {
  dbg::printf("initializing APIC...\n");

  remapDisablePIC();

  auto regs = cpuid(1);
  dbg::panic_assert((regs.rdx>>9)&1, "cpu has no local apic\n");

  auto base = rdmsr(IA32_APIC_BASE_MSR);
  dbg::panic_assert((base>>8)&1, "booting cpu not the BSP\n");
  dbg::panic_assert((base>>11)&1, "local APIC disabled\n");
  dbg::printf("APIC base {}\n", base);
  auto pfn = base/PAGE_SIZE;

  vmm::AddressSpace::setKernelCaching(vmm::pageAddress<size_t>(pfn)/PAGE_SIZE, false);
  apic = vmm::pageAddress<APIC *>(pfn);
  dbg::printf("LAPIC ID: {}\n", apic->id);
  dbg::printf("LAPIC version: {}\n", apic->version);

  apic->spurious_iv = 0x1ff;
  apic->lvt_error = 0xfeU;
  apic->divide_conf = 2;
  apic->lvt_timer = 0x30U | 1U<<17;
  apic->initial_count = 0xffffU;
}

void *searchMPT() {
  uint32_t * const bios_start = reinterpret_cast<uint32_t *>(VIRTUAL_OFFSET+639*1024U);
  for (size_t i = 0; i < 1024ULL/4; ++i) {
    if (bios_start[i] == 0x5f504d5fU) {
      return &bios_start[i];
    }
  }
  uint32_t * const bios_rom = reinterpret_cast<uint32_t *>(VIRTUAL_OFFSET+0xE0000U);
  for (size_t i = 0; i < (1ULL<<17)/4; ++i) {
    if (bios_rom[i] == 0x5f504d5fU) {
      return &bios_rom[i];
    }
  }
  return nullptr;
}

void parseMPT() {

  MP *mp = reinterpret_cast<MP *>(searchMPT());
  dbg::panic_assert(mp->address, "mp config table does not exist\n");
  MPHead *mp_head = reinterpret_cast<MPHead *>(mp->address+VIRTUAL_OFFSET);
  dbg::panic_assert(mp_head->signature_ == 0x504d4350U, "MP config signature incorrect\n");
  size_t cur = mp->address+sizeof(MPHead)+VIRTUAL_OFFSET;
  dbg::printf("MP Table:\n");
  for (size_t i = 0; i < mp_head->entry_cnt_; ++i) {
    uint8_t type = *reinterpret_cast<uint8_t *>(cur);
    switch (type) {
      case 0:
        {
          auto ent = reinterpret_cast<MPProcEntry *>(cur);
          dbg::printf("  CPU {}\n", ent->cpu_sign_);
          cur += sizeof(MPProcEntry);
        }
        break;
      case 1:
        {
          auto ent = reinterpret_cast<MPBusEntry *>(cur);
          char tmp[7];
          memcpy(tmp, ent->string_, 6);
          tmp[6] = 0;
          dbg::printf("  Bus {}\n", tmp);
          cur += sizeof(MPBusEntry);
        }
        break;
      case 2:
        {
          auto ent = reinterpret_cast<MPIOAPICEntry *>(cur);
          dbg::printf("  I/O APIC {}\n", ent->id_);
          cur += sizeof(MPIOAPICEntry);
        }
        break;
      case 3:
        {
          auto ent = reinterpret_cast<MPIOIntEntry *>(cur);
          dbg::printf("  I/O Int {}\n", ent->int_type_);
          cur += sizeof(MPIOIntEntry);
        }
        break;
      case 4:
        {
          auto ent = reinterpret_cast<MPLIntEntry *>(cur);
          dbg::printf("  Local Int {}\n", ent->int_type_);
          cur += sizeof(MPLIntEntry);
        }
        break;
      default:
        dbg::panic("unkown MP entry type\n");
    }
  }
}

RSDP *searchRSDP() {
  uint64_t * const bios_rom = reinterpret_cast<uint64_t *>(VIRTUAL_OFFSET+0xE0000U);
  for (size_t i = 0; i < (1ULL<<17)/8; ++i) {
    if (bios_rom[i] == 0x2052545020445352ULL) {
      return reinterpret_cast<RSDP *>(&bios_rom[i]);
    }
  }
  return nullptr;
}
void parseRSDT() {
  auto rsdp = searchRSDP();
  dbg::panic_assert(rsdp, "RSDP not found\n");

  MADT *apic = nullptr;

  uint64_t phys_addr = rsdp->revision_ == 0 ? rsdp->rsdt_address_ : rsdp->xsdt_address_;
  dbg::printf("RSDT: {}\n", phys_addr);

  if (rsdp->revision_ == 0) {
    auto rsdt = vmm::identAddress<RSDT *>(phys_addr);
    dbg::printf("ACPI description headers:\n");
    for (size_t i = 0; i < (rsdt->length_-sizeof(RSDT))/4; ++i) {
      auto desc = vmm::identAddress<DescHeader *>(rsdt->entry_[i]);
      char tmp[5];
      memcpy(tmp, &desc->signature_, 4);
      tmp[4] = 0;
      dbg::printf("  {}\n", tmp);
      if (desc->signature_ == *reinterpret_cast<const uint32_t *>("APIC"))
        apic = reinterpret_cast<MADT *>(desc);
    }
  }
  else {
    auto xsdt = vmm::identAddress<XSDT *>(phys_addr);
    dbg::printf("ACPI description headers:\n");
    for (size_t i = 0; i < (xsdt->length_-sizeof(XSDT))/8; ++i) {
      auto desc = vmm::identAddress<DescHeader *>(xsdt->entry_[i]);
      char tmp[5];
      memcpy(tmp, &desc->signature_, 4);
      tmp[4] = 0;
      dbg::printf("  {}\n", tmp);
      if (desc->signature_ == *reinterpret_cast<const uint32_t *>("APIC"))
        apic = reinterpret_cast<MADT *>(desc);
    }
  }
  dbg::panic_assert(apic, "no apic descriptor found\n");
  
  dbg::printf("MADT:\n");
  uint64_t apic_s = reinterpret_cast<uint64_t>(apic);
  for (size_t i = sizeof(MADT); i < apic->length_;) {
    auto curr = reinterpret_cast<MADTEntry *>(apic_s + i);
    switch (curr->type_) {
      case 0:
        {
          auto curr = reinterpret_cast<MADTLAPICEntry *>(apic_s + i);
          dbg::printf("  LAPIC: {}\n", curr->apic_id_);

        }
        break;
      default:
        dbg::printf("  unkown type: {}\n", curr->type_);
        break;
    }
    i += curr->length_;

  }









}

}
