#include "stdint.h"
#include "debug.h"
#include "utility.h"
#include "gdt.h"
#include "asm.h"
#include "vmm.h"
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

}
