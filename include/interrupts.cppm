module;
#include "stdint.h"
#include "stddef.h"

export module interrupts;
import gdt;
import debug;
import vmm;
import utility;
import array;

export extern "C" void context_return();
export extern "C" void context_switch(void *dest, void *src);

export namespace irq {
  void initIdt();
  void initAPIC();
  void parseMPT();
  void parseRSDT();
  void remapDisablePIC();
  void launchCores();

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
    uint8_t apic_id_;
    array<pair<uint32_t, uint32_t>> remaps_;

    void write(uint32_t reg, uint32_t val) {
      *addr_ = reg;
      *(addr_+4) = val;
    }
    uint32_t read(uint32_t reg) {
      *addr_ = reg;
      return *(addr_+4);
    }
  
    IOAPIC(void *addr) : addr_((uint32_t *)addr) { 
      uint32_t ioapicid = read(0);
      apic_id_ = ioapicid>> 24 &0xf;
      dbg::printf("IO APIC ID: {}\n", ioapicid);
      dbg::printf("  IO APIC version: {}\n", read(1));
    }
  };
}

export struct ArchCPU {
  constexpr ArchCPU() = default;
  TSS tss;
  uint64_t cr3;
  vmm::AddressSpace *address_space = nullptr;
  GDTE *gdt = nullptr;
};
