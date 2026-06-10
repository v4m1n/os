module;
#include <cstdint>

export module acpi;

export {

  struct RSDP {
    uint8_t signature_[8];
    uint8_t checksum_;
    uint8_t oem_id_[6];
    uint8_t revision_;
    uint32_t rsdt_address_;
    uint32_t length;
    uint64_t xsdt_address_;
    uint8_t ext_checksum_;
    uint8_t reserved_[3];
  } __attribute__((packed));

  static_assert(sizeof(RSDP) == 36);

  struct RSDT {
    uint32_t signature_;
    uint32_t length_;
    uint8_t revision_;
    uint8_t checksum_;
    uint8_t oem_id_[6];
    uint8_t oem_table_id_[8];
    uint32_t oem_revision_;
    uint32_t creator_id_;
    uint32_t creator_revision_;
    uint32_t entry_[];
  } __attribute__((packed));

  static_assert(sizeof(RSDT) == 36);

  struct XSDT {
    uint32_t signature_;
    uint32_t length_;
    uint8_t revision_;
    uint8_t checksum_;
    uint8_t oem_id_[6];
    uint8_t oem_table_id_[8];
    uint32_t oem_revision_;
    uint32_t creator_id_;
    uint32_t creator_revision_;
    uint64_t entry_[];
  } __attribute__((packed));

  static_assert(sizeof(XSDT) == 36);

  struct DescHeader {
    uint32_t signature_;
    uint32_t length_;
    uint8_t revision_;
    uint8_t checksum_;
    uint8_t oem_id_[6];
    uint8_t oem_table_id_[8];
    uint32_t oem_revision_;
    uint32_t creator_id_;
    uint32_t creator_revision_;
  } __attribute__((packed));

  static_assert(sizeof(DescHeader) == 36);

  struct MADT {
    uint32_t signature_;
    uint32_t length_;
    uint8_t revision_;
    uint8_t checksum_;
    uint8_t oem_id_[6];
    uint8_t oem_table_id_[8];
    uint32_t oem_revision_;
    uint32_t creator_id_;
    uint32_t creator_revision_;
    uint32_t lapic_address_;
    uint32_t flags_;
  } __attribute__((packed));

  static_assert(sizeof(MADT) == 44);

  struct MADTEntry {
    uint8_t type_;
    uint8_t length_;
  } __attribute__((packed));

  static_assert(sizeof(MADTEntry) == 2);

  struct MADTLAPICEntry {
    uint8_t type_;
    uint8_t length_;
    uint8_t acpi_proc_uid_;
    uint8_t apic_id_;
    uint8_t flags_;
  } __attribute__((packed));

  static_assert(sizeof(MADTLAPICEntry) == 5);

  struct MADTIOAPICEntry {
    uint8_t type_;
    uint8_t length_;
    uint8_t ioapic_id_;
    uint8_t reserved_;
    uint32_t address_;
    uint32_t irq_base_;
  } __attribute__((packed));

  static_assert(sizeof(MADTIOAPICEntry) == 12);

  struct MADTIOAPICSourceEntry {
    uint8_t type_;
    uint8_t length_;
    uint8_t bus_src_;
    uint8_t irq_src_;
    uint32_t irq_;
    uint16_t flags_;
  } __attribute__((packed));

  static_assert(sizeof(MADTIOAPICSourceEntry) == 10);

}
