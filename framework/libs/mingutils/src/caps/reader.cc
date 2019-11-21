#include <string.h>
#include <arpa/inet.h>
#include "writer.h"
#include "reader.h"

using namespace std;

namespace rokid {

static int64_t caps_ntohll(int64_t v) {
  int32_t num = 42;
  if (*(char*)&num == 42) {
    return ((((uint64_t)ntohl(v)) << 32) + ntohl((v) >> 32));
  }
  return v;
}

static void copy_from_reader(CapsReader* dst, const CapsReader* src) {
  const void* data = src->binary_data();
  uint32_t size = src->binary_size();
  if (data != nullptr && size > 0) {
    dst->parse(data, size, true);
  }
}

static void copy_from_writer(CapsReader* dst, const CapsWriter* src) {
  uint32_t size = src->binary_size();
  if (size > 0) {
    int8_t* data = new int8_t[size];
    if (src->serialize(data, size, 0) > 0) {
      dst->parse(data, size, true);
    }
    delete[] data;
  }
}

CapsReader& CapsReader::operator = (const Caps& o) {
  if (o.type() == CAPS_TYPE_WRITER)
    copy_from_writer(this, static_cast<const CapsWriter*>(&o));
  else
    copy_from_reader(this, static_cast<const CapsReader*>(&o));
  return *this;
}

int32_t CapsReader::parse(const void* data, uint32_t datasize, bool dup) {
  uint8_t num_str = 0;
  uint8_t num_bin = 0;
  uint8_t num_obj = 0;
  uint8_t num_num = 0;
  uint8_t num_long = 0;
  uint8_t i;
  uint8_t num_members;
  const int8_t* b;

  if (datasize <= sizeof(Header))
    return CAPS_ERR_INVAL;
  if (dup && bin_data)
    delete bin_data;
  if (dup) {
    bin_data = new int8_t[datasize];
    memcpy(const_cast<int8_t*>(bin_data), data, datasize);
    b = reinterpret_cast<const int8_t*>(bin_data);
  } else {
    b = reinterpret_cast<const int8_t*>(data);
    bin_data = b;
  }
  duplicated = dup;

  header = reinterpret_cast<const Header*>(b);
  int32_t r = check_header(header, data_length);
  if (r)
    return r;
  if (data_length != datasize)
    return CAPS_ERR_CORRUPTED;

  member_declarations = reinterpret_cast<const char*>(b + datasize);
  --member_declarations;
  num_members = (uint8_t)member_declarations[0];
  if (datasize < sizeof(Header) + ALIGN4(num_members + 1))
    return CAPS_ERR_CORRUPTED;
  --member_declarations;
  for (i = 0; i < num_members; ++i) {
    switch (member_declarations[-(int32_t)i]) {
      case 'i':
      case 'f':
        ++num_num;
        break;
      case 'l':
      case 'd':
        ++num_long;
        break;
      case 'S':
        ++num_str;
        break;
      case 'B':
        ++num_bin;
        break;
      case 'O':
        ++num_bin;
        ++num_obj;
        break;
      case 'V':
        break;
      default:
        return CAPS_ERR_CORRUPTED;
    }
  }
  long_values = reinterpret_cast<const int64_t*>(header + 1);
  number_values = reinterpret_cast<const int32_t*>(long_values + num_long);
  bin_sizes = reinterpret_cast<const uint32_t*>(number_values + num_num);
  binary_section = reinterpret_cast<const int8_t*>(bin_sizes + num_bin);
  if (datasize < reinterpret_cast<const int8_t*>(binary_section) - b)
    return CAPS_ERR_CORRUPTED;
  uint32_t bin_sec_size = 0;
  uint32_t bin_size;
  for (i = 0; i < num_bin; ++i) {
    if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
      bin_size = ntohl(bin_sizes[i]);
    else
      bin_size = bin_sizes[i];
    bin_sec_size += ALIGN4(bin_size);
  }
  string_section = reinterpret_cast<const char*>(binary_section + bin_sec_size);
  // TODO: 检查 string section 与 member declarations 不重叠
  return CAPS_SUCCESS;
}

uint32_t CapsReader::binary_size() const {
  return bin_data ? data_length : 0;
}

int32_t CapsReader::read32(int32_t* r, char type) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != type)
    return CAPS_ERR_INCORRECT_TYPE;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    *r = ntohl(number_values[0]);
  else
    *r = number_values[0];
  ++number_values;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read64(int64_t* r, char type) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != type)
    return CAPS_ERR_INCORRECT_TYPE;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    *r = caps_ntohll(long_values[0]);
  else
    *r = long_values[0];
  ++long_values;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(int32_t& r) {
  return read32(&r, 'i');
}

int32_t CapsReader::read(uint32_t& r) {
  return read32(reinterpret_cast<int32_t*>(&r), 'i');
}

int32_t CapsReader::read(float& r) {
  return read32(reinterpret_cast<int32_t*>(&r), 'f');
}

int32_t CapsReader::read(int64_t& r) {
  return read64(&r, 'l');
}

int32_t CapsReader::read(uint64_t& r) {
  return read64(reinterpret_cast<int64_t*>(&r), 'l');
}

int32_t CapsReader::read(double& r) {
  return read64(reinterpret_cast<int64_t*>(&r), 'd');
}

int32_t CapsReader::read(const char*& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'S')
    return CAPS_ERR_INCORRECT_TYPE;
  r = string_section;
  string_section += strlen(r) + 1;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(const void*& r, uint32_t& length) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'B')
    return CAPS_ERR_INCORRECT_TYPE;
  r = binary_section;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    length = ntohl(bin_sizes[0]);
  else
    length = bin_sizes[0];
  binary_section += ALIGN4(length);
  ++bin_sizes;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(string& r) {
  const char* s;
  int32_t code = read(s);
  if (code != CAPS_SUCCESS)
    return code;
  r = s;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(vector<uint8_t>& r) {
  const void* b;
  uint32_t l;
  int32_t code = read(b, l);
  if (code != CAPS_SUCCESS)
    return code;
  r.resize(l);
  memcpy(r.data(), b, l);
  return CAPS_SUCCESS;
}

int32_t CapsReader::read_string(string& r) {
  return read(r);
}

int32_t CapsReader::read_binary(string& r) {
  const void* b;
  uint32_t l;
  int32_t code = read(b, l);
  if (code != CAPS_SUCCESS)
    return code;
  r.assign(reinterpret_cast<const char*>(b), l);
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(shared_ptr<Caps>& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'O')
    return CAPS_ERR_INCORRECT_TYPE;

  shared_ptr<CapsReader> sub;
  int32_t code = CAPS_SUCCESS;
  uint32_t bin_size;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    bin_size = ntohl(bin_sizes[0]);
  else
    bin_size = bin_sizes[0];
  if (bin_size > 0) {
    sub = make_shared<CapsReader>();
    code = sub->parse(binary_section, bin_size, true);
  }
  binary_section += bin_size;
  ++bin_sizes;
  ++current_read_member;
  r = static_pointer_cast<Caps>(sub);
  return code;
}

int32_t CapsReader::read() {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'V')
    return CAPS_ERR_INCORRECT_TYPE;
  ++current_read_member;
  return CAPS_SUCCESS;
}

CapsReader::~CapsReader() noexcept {
  if (duplicated)
    delete[] bin_data;
}

int8_t CapsReader::current_member_type() const {
  if (end_of_object())
    return '\0';
  return member_declarations[-(int32_t)current_read_member];
}

bool CapsReader::end_of_object() const {
  return (uint8_t)member_declarations[1] <= current_read_member;
}

void CapsReader::record(CapsReaderRecord& rec) const {
  rec.number_values = number_values;
  rec.long_values = long_values;
  rec.bin_sizes = bin_sizes;
  rec.binary_section = binary_section;
  rec.string_section = string_section;
  rec.current_read_member = current_read_member;
}

void CapsReader::rollback(const CapsReaderRecord& rec) {
  number_values = rec.number_values;
  long_values = rec.long_values;
  bin_sizes = rec.bin_sizes;
  binary_section = rec.binary_section;
  string_section = rec.string_section;
  current_read_member = rec.current_read_member;
}

uint32_t CapsReader::size() const {
  return member_declarations[1];
}

int32_t CapsReader::next_type() const {
  int32_t r = current_member_type();
  if (r == '\0')
    return CAPS_ERR_EOO;
  return r;
}

} // namespace rokid
