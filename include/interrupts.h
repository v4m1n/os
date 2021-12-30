#pragma once


extern "C" void context_return();
extern "C" void context_switch(void *dest, void *src);

namespace irq {
  void initIdt();
  void initAPIC();
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

}
