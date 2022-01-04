#pragma once
#include "stdint.h"

struct MP {
  uint32_t signature_;
  uint32_t address;
  uint8_t len_;
  uint8_t rev_;
  uint8_t checksum_;
  uint8_t feature1_;
  uint32_t feature2_;
};
struct MPHead {
  uint32_t signature_;
  uint8_t len_;
  uint8_t spec_;
  uint8_t oem_string[20];
  uint32_t oem_table_ptr_;
  uint16_t oem_table_sz_;
  uint16_t entry_cnt_;
  uint32_t lapic_addr_;
  uint16_t ext_table_len_;
  uint8_t ext_checksum_;
  uint8_t reserved_;
};
struct MPProcEntry {
  uint8_t type_;
  uint8_t lapic_id_;
  uint8_t lapic_version_;
  uint8_t cpu_flags_;
  uint32_t cpu_sign_;
  uint32_t feature_flags_;
  uint32_t reserved1_;
  uint32_t reserved2_;
};
struct MPBusEntry {
  uint8_t type_;
  uint8_t buts_id_;
  uint8_t string_[6];
};
struct MPIOAPICEntry {
  uint8_t type_;
  uint8_t id_;
  uint8_t flags_;
  uint32_t address_;
};
struct MPIOIntEntry {
  uint8_t type_;
  uint8_t int_type_;
  uint16_t flags_;
  uint8_t src_id_;
  uint8_t src_irq_;
  uint8_t dst_id_;
  uint8_t dst_irq_;
};
struct MPLIntEntry {
  uint8_t type_;
  uint8_t int_type_;
  uint16_t flags_;
  uint8_t src_id_;
  uint8_t src_irq_;
  uint8_t dst_id_;
  uint8_t dst_irq_;
};

static_assert(sizeof(MP) == 4*4);
static_assert(sizeof(MPHead) == 4*11);
static_assert(sizeof(MPBusEntry) == 4*2);
static_assert(sizeof(MPProcEntry) == 4*5);
static_assert(sizeof(MPIOAPICEntry) == 4*2);
static_assert(sizeof(MPIOIntEntry) == 4*2);
static_assert(sizeof(MPLIntEntry) == 4*2);
