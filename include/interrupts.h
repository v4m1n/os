#pragma once
#include "gdt.h"
#include "vmm.h"

extern "C" void context_return();
extern "C" void context_switch(void *dest, void *src);

namespace irq {
  void initIdt();
  void initAPIC();
  void parseMPT();
  void parseRSDT();
  void remapDisablePIC();
  void launchCores();

  inline bool getIF() {
    uint64_t flags;
    asm volatile("pushf\npop %0" :"=r"(flags):: "memory");
    return flags &0x200;
  }
  inline void enableInterrupts() { asm volatile("sti" ::: "memory"); }
  inline void disableInterrupts() { asm volatile("cli" ::: "memory"); }

  struct aligned_uint32 {
    alignas(16) volatile uint32_t data;
  };
  
  struct APIC {
    aligned_uint32 reserved1[2];
    alignas(16) volatile uint32_t id;
    alignas(16) volatile uint32_t version;
    aligned_uint32 reserved2[4];
    alignas(16) volatile uint32_t task_priority;
    alignas(16) volatile uint32_t arbitration_priority;
    alignas(16) volatile uint32_t processor_priority;
    alignas(16) volatile uint32_t EOI;
    alignas(16) volatile uint32_t remote_read;
    alignas(16) volatile uint32_t logical_dest;
    alignas(16) volatile uint32_t dest_format;
    alignas(16) volatile uint32_t spurious_iv;
    aligned_uint32 in_service[8];
    aligned_uint32 trigger_mode[8];
    aligned_uint32 interrupt_req[8];
    alignas(16) volatile uint32_t error_status;
    aligned_uint32 reserved3[6];
    alignas(16) volatile uint32_t CMCI;
    aligned_uint32 interrupt_command[2];
    alignas(16) volatile uint32_t lvt_timer;
    alignas(16) volatile uint32_t lvt_thermal_sensor;
    alignas(16) volatile uint32_t lvt_pref_counters;
    alignas(16) volatile uint32_t lvt_lint0;
    alignas(16) volatile uint32_t lvt_lint1;
    alignas(16) volatile uint32_t lvt_error;
    alignas(16) volatile uint32_t initial_count;
    alignas(16) volatile uint32_t current_count;
    aligned_uint32 reserved4[4];
    alignas(16) volatile uint32_t divide_conf;
    alignas(16) volatile uint32_t reserved5;
  };
  
  static_assert(sizeof(APIC) == 0x400);

  struct IOAPIC {
    union RedirectionEntry
    {
      struct
      {
        uint64_t vector       : 8;
        uint64_t delvMode     : 3;
        uint64_t destMode     : 1;
        uint64_t delvStatus   : 1;
        uint64_t pinPolarity  : 1;
        uint64_t remoteIRR    : 1;
        uint64_t triggerMode  : 1;
        uint64_t mask         : 1;
        uint64_t reserved     : 39;
        uint64_t destination  : 8;
      };
      struct
      {
        uint32_t low;
        uint32_t high;
      };
    };
    volatile uint32_t *addr_;
    IOAPIC(uint32_t *addr) : addr_(addr) {}

    uint32_t read(unsigned char regOff) {
      addr_[0] = regOff;
      return addr_[4];
    }

    void write(unsigned char regOff, uint32_t data) {
      addr_[0] = regOff;
      addr_[4] = data;
    }


    RedirectionEntry readEntry(uint8_t nr) {
      RedirectionEntry entry;
      entry.low = read(0x10+nr*2);
      entry.high = read(0x10+nr*2+1);
      return entry;
    }
    void writeEntry(uint8_t nr, RedirectionEntry entry) {
      write(0x10+nr*2, entry.low);
      write(0x10+nr*2+1, entry.high);
    }
  };

}
struct ArchCPU {
  constexpr ArchCPU() = default;
  TSS tss;
  uint64_t cr3;
  vmm::AddressSpace *address_space = nullptr;
  GDTE *gdt = nullptr;
};
