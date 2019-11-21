#pragma once

#include <stdint.h>

#define ALIGN4(v) ((v) + 3 & ~3)
#define ALIGN8(v) ((v) + 7 & ~7)
#define CAPS_MAGIC_MASK 0x1f

namespace rokid {

typedef struct {
  // magic[0]: 0x1e | FLAG_NET_BYTEORDER(0x80)
  // magic[1]: 'A'
  // magic[2]: 'P'
  // magic[3]: CAPS_VERSION
  char magic[4];
  uint32_t length;
} Header;

typedef struct {
  const int32_t* number_values;
  const int64_t* long_values;
  const uint32_t* bin_sizes;
  const int8_t* binary_section;
  const char* string_section;
  uint32_t current_read_member;
} CapsReaderRecord;

int32_t check_header(const Header* header, uint32_t& length);

extern char CAPS_MAGIC[4];

} // namespace rokid
