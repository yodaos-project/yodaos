#pragma once

#include <stdint.h>
#include <vector>
#include "defs.h"
#include "caps.h"

namespace rokid {

class Member;

class CapsWriter : public Caps {
public:
  CapsWriter();

  ~CapsWriter() noexcept;

  CapsWriter& operator = (const Caps& o);

  // override from 'Caps'
  int32_t write(int32_t v);
  int32_t write(uint32_t v);
  int32_t write(int64_t v);
  int32_t write(uint64_t v);
  int32_t write(float v);
  int32_t write(double v);
  int32_t write(const char* v);
  int32_t write(const std::string& v);
  int32_t write(const void* v, uint32_t l);
  int32_t write(const std::vector<uint8_t>& v);
  int32_t write(std::shared_ptr<Caps>& v);
  int32_t write();
  int32_t serialize(void* buf, uint32_t bufsize, uint32_t flags) const;

  int32_t read(int32_t& v) { return CAPS_ERR_WRONLY; }
  int32_t read(uint32_t& v) { return CAPS_ERR_WRONLY; }
  int32_t read(float& v) { return CAPS_ERR_WRONLY; }
  int32_t read(int64_t& v) { return CAPS_ERR_WRONLY; }
  int32_t read(uint64_t& v) { return CAPS_ERR_WRONLY; }
  int32_t read(double& v) { return CAPS_ERR_WRONLY; }
  int32_t read(const char*& v) { return CAPS_ERR_WRONLY; }
  int32_t read(std::string& v) { return CAPS_ERR_WRONLY; }
  int32_t read(const void*& r, uint32_t& size) { return CAPS_ERR_WRONLY; }
  int32_t read(std::vector<uint8_t>& v) { return CAPS_ERR_WRONLY; }
  int32_t read_string(std::string& v) { return CAPS_ERR_WRONLY; }
  int32_t read_binary(std::string& v) { return CAPS_ERR_WRONLY; }
  int32_t read(std::shared_ptr<Caps>& v) { return CAPS_ERR_WRONLY; }
  int32_t read() { return CAPS_ERR_WRONLY; }

  int32_t type() const { return CAPS_TYPE_WRITER; }
  uint32_t binary_size() const;
  uint32_t size() const;
  int32_t next_type() const { return CAPS_ERR_WRONLY; }

private:
  void copy_from_writer(CapsWriter* dst, const CapsWriter* src);

private:
  std::vector<Member*> members;
  std::vector<std::shared_ptr<Caps> > sub_objects;
  uint32_t number_member_number = 0;
  uint32_t long_member_number = 0;
  uint32_t string_member_number = 0;
  uint32_t binary_object_member_number = 0;
  uint32_t binary_section_size = 0;
  uint32_t string_section_size = 0;
  mutable uint32_t object_data_size = 0;
};

} // namespace rokid
