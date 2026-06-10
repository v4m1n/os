module;
#include "stdint.h"

export module gdt;

export struct GDTE {
  uint16_t limit1;
  uint16_t base1;
  uint8_t base2;
  uint8_t accessed : 1;
  uint8_t rw : 1;
  uint8_t direction : 1;
  uint8_t executable : 1;
  uint8_t descriptor : 1;
  uint8_t priv : 2;
  uint8_t present : 1;
  uint8_t limit2 : 4;
  uint8_t zero : 1;
  uint8_t lmode : 1;
  uint8_t size : 1;
  uint8_t granularity : 1;
  uint8_t base3;
  inline void setBase(uint32_t addr) {
    base1 = (uint16_t)addr;
    base2 = (uint8_t)(addr>>16);
    base3 = (uint8_t)(addr>>24);
  }
} __attribute__((packed));

export struct TSS {
  uint32_t reserved1;
  uint32_t rsp0_low;
  uint32_t rsp0_high;
  uint32_t rsp1_low;
  uint32_t rsp1_high;
  uint32_t rsp2_low;
  uint32_t rsp2_high;
  uint32_t reserved2[2];
  uint32_t ist1_low;
  uint32_t ist1_high;
  uint32_t ist2_low;
  uint32_t ist2_high;
  uint32_t ist3_low;
  uint32_t ist3_high;
  uint32_t ist4_low;
  uint32_t ist4_high;
  uint32_t ist5_low;
  uint32_t ist5_high;
  uint32_t ist6_low;
  uint32_t ist6_high;
  uint32_t ist7_low;
  uint32_t ist7_high;
  uint16_t reserved3[5]; 
  uint16_t iopb_offset;
} __attribute__((packed));

extern "C" {
export extern GDTE gdt[7];
export extern TSS tss;
}

export constexpr uint16_t KERNEL_CS = 0x8;
export constexpr uint16_t KERNEL_DS = 0x10;
export constexpr uint16_t USER_CS = 0x1b;
export constexpr uint16_t USER_DS = 0x23;
export constexpr uint16_t TSSS = 0x28;
