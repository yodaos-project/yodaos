#pragma once

#include <stdint.h>
#include "defs.h"
#include "caps.h"

namespace rokid {

class CapsReader : public Caps {
public:
  ~CapsReader() noexcept;

  CapsReader& operator = (const Caps& o);

  int32_t parse(const void* data, uint32_t datasize, bool dup);

  inline const void* binary_data() const { return bin_data; }

  // override from 'Caps'
  int32_t write(int32_t v) { return CAPS_ERR_RDONLY; }
  int32_t write(uint32_t v) { return CAPS_ERR_RDONLY; }
  int32_t write(float v) { return CAPS_ERR_RDONLY; }
  int32_t write(int64_t v) { return CAPS_ERR_RDONLY; }
  int32_t write(uint64_t v) { return CAPS_ERR_RDONLY; }
  int32_t write(double v) { return CAPS_ERR_RDONLY; }
  int32_t write(const char* v) { return CAPS_ERR_RDONLY; }
  int32_t write(const std::string& v) { return CAPS_ERR_RDONLY; }
  int32_t write(const void* v, uint32_t len) { return CAPS_ERR_RDONLY; }
  int32_t write(const std::vector<uint8_t>& v) { return CAPS_ERR_RDONLY; }
  int32_t write(std::shared_ptr<Caps>& v) { return CAPS_ERR_RDONLY; }
  int32_t write() { return CAPS_ERR_RDONLY; }
  int32_t serialize(void* buf, uint32_t size, uint32_t flags) const { return CAPS_ERR_RDONLY; }

  int32_t read(int32_t& r);
  int32_t read(uint32_t& r);
  int32_t read(float& r);
  int32_t read(int64_t& r);
  int32_t read(uint64_t& r);
  int32_t read(double& r);
  int32_t read(std::string& r);
  int32_t read(std::vector<uint8_t>& r);
  int32_t read_string(std::string& r);
  int32_t read_binary(std::string& r);
  int32_t read(std::shared_ptr<Caps>& r);
  int32_t read();
  int32_t next_type() const;

  // read string & binary without memcpy
  int32_t read(const char*& r);
  int32_t read(const void*& r, uint32_t& len);

  int32_t type() const { return CAPS_TYPE_READER; }
  uint32_t binary_size() const;
  uint32_t size() const;

  int8_t current_member_type() const;
  bool end_of_object() const;
  void record(CapsReaderRecord& rec) const;
  void rollback(const CapsReaderRecord& rec);

private:
  int32_t read32(int32_t* r, char type);
  int32_t read64(int64_t* r, char type);

private:
  const Header* header = nullptr;
  const char* member_declarations = nullptr;
  const int32_t* number_values = nullptr;
  const int64_t* long_values = nullptr;
  const uint32_t* bin_sizes = nullptr;
  const int8_t* binary_section = nullptr;
  const char* string_section = nullptr;
  uint32_t current_read_member = 0;
  uint32_t data_length = 0;
  bool duplicated = false;

  const int8_t* bin_data = nullptr;
};

} // namespace rokid
