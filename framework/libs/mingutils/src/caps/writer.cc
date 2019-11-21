#include <string.h>
#include <arpa/inet.h>
#include "writer.h"
#include "reader.h"

using namespace std;

namespace rokid {

static int64_t caps_htonll(int64_t v) {
  int32_t num = 42;
  if (*(char*)&num == 42) {
    return ((((uint64_t)htonl(v)) << 32) + htonl((v) >> 32));
  }
  return v;
}

typedef struct {
  char* mdecls;
  int32_t* ivalues;
  int64_t* lvalues;
  uint32_t* bin_sizes;
  int8_t* bin_section;
  char* str_section;

  uint32_t cur_strp = 0;
  uint32_t cur_binp = 0;
} WritePointer;

class Member {
public:
  virtual ~Member() = default;

  virtual void do_serialize(Header* header, WritePointer* wp) const = 0;

  virtual char type() const = 0;
};

class IntegerMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const;

  char type() const { return 'i'; }

  int32_t value;
};

class LongMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const;

  char type() const { return 'l'; }

  int64_t value;
};

class FloatMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const;

  char type() const { return 'f'; }

  float value;
};

class DoubleMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const;

  char type() const { return 'd'; }

  double value;
};

class StringMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const;

  char type() const { return 'S'; }

  string value;
};

class BinaryMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const;

  char type() const { return 'B'; }

  string value;
};

class ObjectMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const;

  char type() const { return 'O'; }

  shared_ptr<Caps> value;
};

class VoidMember : public Member {
public:
  void do_serialize(Header* header, WritePointer* wp) const {
    wp->mdecls[0] = 'V';
    --wp->mdecls;
  }

  char type() const { return 'V'; }
};

void IntegerMember::do_serialize(Header* header, WritePointer* wp) const {
  wp->mdecls[0] = 'i';
  --wp->mdecls;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    wp->ivalues[0] = htonl(value);
  else
    wp->ivalues[0] = value;
  ++wp->ivalues;
}

void FloatMember::do_serialize(Header* header, WritePointer* wp) const {
  wp->mdecls[0] = 'f';
  --wp->mdecls;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    wp->ivalues[0] = htonl(*((int32_t*)&value));
  else
    memcpy(wp->ivalues, &value, sizeof(value));
  ++wp->ivalues;
}

void LongMember::do_serialize(Header* header, WritePointer* wp) const {
  wp->mdecls[0] = 'l';
  --wp->mdecls;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    wp->lvalues[0] = caps_htonll(value);
  else
    wp->lvalues[0] = value;
  ++wp->lvalues;
}

void DoubleMember::do_serialize(Header* header, WritePointer* wp) const {
  wp->mdecls[0] = 'd';
  --wp->mdecls;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    wp->lvalues[0] = caps_htonll(*(int64_t*)(&value));
  else
    memcpy(wp->lvalues, &value, sizeof(value));
  ++wp->lvalues;
}

void StringMember::do_serialize(Header* header, WritePointer* wp) const {
  wp->mdecls[0] = 'S';
  --wp->mdecls;
  memcpy(wp->str_section + wp->cur_strp, value.c_str(), value.length() + 1);
  wp->cur_strp += value.length() + 1;
}

void BinaryMember::do_serialize(Header* header, WritePointer* wp) const {
  wp->mdecls[0] = 'B';
  --wp->mdecls;
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    wp->bin_sizes[0] = htonl(value.length());
  else
    wp->bin_sizes[0] = value.length();
  ++wp->bin_sizes;
  if (value.length() > 0) {
    memcpy(wp->bin_section + wp->cur_binp, value.data(), value.length());
    wp->cur_binp += ALIGN4(value.length());
  }
}

void ObjectMember::do_serialize(Header* header, WritePointer* wp) const {
  int32_t obj_size;
  uint32_t flags = header->magic[0] & CAPS_FLAG_NET_BYTEORDER;

  wp->mdecls[0] = 'O';
  --wp->mdecls;
  if (value.get())
    obj_size = value->binary_size();
  else
    obj_size = 0;
  if (value.get()) {
    if (value->type() == CAPS_TYPE_WRITER) {
      static_pointer_cast<CapsWriter>(value)->serialize(wp->bin_section + wp->cur_binp, obj_size, flags);
    } else {
      memcpy(wp->bin_section + wp->cur_binp, static_pointer_cast<CapsReader>(value)->binary_data(), obj_size);
    }
  }
  if (flags & CAPS_FLAG_NET_BYTEORDER)
    wp->bin_sizes[0] = htonl(obj_size);
  else
    wp->bin_sizes[0] = obj_size;
  ++wp->bin_sizes;
  wp->cur_binp += ALIGN4(obj_size);
}

CapsWriter::CapsWriter() {
  members.reserve(8);
}

CapsWriter::~CapsWriter() noexcept {
  int32_t i;
  for (i = 0; i < members.size(); ++i) {
    delete members[i];
  }
}

int32_t CapsWriter::write(int32_t v) {
  IntegerMember* m = new IntegerMember();
  m->value = v;
  members.push_back(m);
  ++number_member_number;
  return CAPS_SUCCESS;
}

int32_t CapsWriter::write(uint32_t v) {
  return write((int32_t)v);
}

int32_t CapsWriter::write(int64_t v) {
  LongMember* m = new LongMember();
  m->value = v;
  members.push_back(m);
  ++long_member_number;
  return CAPS_SUCCESS;
}

int32_t CapsWriter::write(uint64_t v) {
  return write((int64_t)v);
}

int32_t CapsWriter::write(float v) {
  FloatMember* m = new FloatMember();
  m->value = v;
  members.push_back(m);
  ++number_member_number;
  return CAPS_SUCCESS;
}

int32_t CapsWriter::write(double v) {
  DoubleMember* m = new DoubleMember();
  m->value = v;
  members.push_back(m);
  ++long_member_number;
  return CAPS_SUCCESS;
}

int32_t CapsWriter::write(const char* v) {
  StringMember* m = new StringMember();
  m->value = v;
  members.push_back(m);
  ++string_member_number;
  string_section_size += m->value.length() + 1;
  return CAPS_SUCCESS;
}

int32_t CapsWriter::write(const string& v) {
  return write(v.c_str());
}

int32_t CapsWriter::write(const void* v, uint32_t l) {
  if (v == nullptr && l > 0)
    return CAPS_ERR_INVAL;
  BinaryMember* m = new BinaryMember();
  if (v)
    m->value.assign((const char*)v, l);
  members.push_back(m);
  ++binary_object_member_number;
  binary_section_size += ALIGN4(l);
  return CAPS_SUCCESS;
}

int32_t CapsWriter::write(const vector<uint8_t>& v) {
  return write(v.data(), v.size());
}

int32_t CapsWriter::write(shared_ptr<Caps>& v) {
  ObjectMember* m = new ObjectMember();
  m->value = v;
  members.push_back(m);
  ++binary_object_member_number;
  sub_objects.push_back(v);
  return CAPS_SUCCESS;
}

int32_t CapsWriter::write() {
  VoidMember* m = new VoidMember();
  members.push_back(m);
  return CAPS_SUCCESS;
}

uint32_t CapsWriter::binary_size() const {
  uint32_t r;
  size_t i;
  
  r = sizeof(Header);
  r += long_member_number * sizeof(int64_t); // long section
  r += number_member_number * sizeof(uint32_t); // number section
  r += binary_object_member_number * sizeof(uint32_t); // binary sizes
  r += binary_section_size;
  // sub objects
  object_data_size = 0;
  for (i = 0; i < sub_objects.size(); ++i) {
    if (sub_objects[i].get())
      object_data_size += sub_objects[i]->binary_size();
  }
  r += object_data_size;
  r += string_section_size;
  r += members.size() + 1; // member declarations
  return ALIGN4(r);
}

int32_t CapsWriter::serialize(void* buf, uint32_t bufsize,
    uint32_t flags) const {
  Header* header;
  WritePointer wp;
  uint32_t total_size = binary_size();

  if (bufsize < total_size || buf == nullptr)
    return total_size;
  header = reinterpret_cast<Header*>(buf);
  wp.lvalues = reinterpret_cast<int64_t*>(header + 1);
  wp.ivalues = reinterpret_cast<int32_t*>(wp.lvalues + long_member_number);
  wp.bin_sizes = reinterpret_cast<uint32_t*>(wp.ivalues + number_member_number);
  wp.bin_section = reinterpret_cast<int8_t*>(wp.bin_sizes + binary_object_member_number);
  wp.str_section = reinterpret_cast<char*>(wp.bin_section + binary_section_size + object_data_size);
  wp.mdecls = reinterpret_cast<char*>(buf) + total_size - 1;

  memcpy(header->magic, CAPS_MAGIC, sizeof(CAPS_MAGIC));
  if (flags & CAPS_FLAG_NET_BYTEORDER) {
    header->magic[0] |= CAPS_FLAG_NET_BYTEORDER;
    header->length = htonl(total_size);
  } else {
    header->length = total_size;
  }
  wp.mdecls[0] = members.size();
  --wp.mdecls;

  size_t i;
  for (i = 0; i < members.size(); ++i) {
    members[i]->do_serialize(header, &wp);
  }
  return total_size;
}

static void copy_from_reader(CapsWriter* dst, const CapsReader* src) {
  int32_t iv;
  float fv;
  int64_t lv;
  double dv;
  const char* sv;
  const void* bv;
  uint32_t bl;
  shared_ptr<Caps> cv;
  CapsReaderRecord rec;
  CapsReader* msrc = const_cast<CapsReader*>(src);

  msrc->record(rec);
  while (!msrc->end_of_object()) {
    switch (msrc->current_member_type()) {
      case 'i':
        msrc->read(iv);
        dst->write(iv);
        break;
      case 'f':
        msrc->read(fv);
        dst->write(fv);
        break;
      case 'l':
        msrc->read(lv);
        dst->write(lv);
        break;
      case 'd':
        msrc->read(dv);
        dst->write(dv);
        break;
      case 'S':
        msrc->read(sv);
        dst->write(sv);
        break;
      case 'B':
        msrc->read(bv, bl);
        dst->write(bv, bl);
        break;
      case 'O':
        msrc->read(cv);
        dst->write(cv);
        break;
      case 'V':
        msrc->read();
        dst->write();
        break;
    }
  }
  msrc->rollback(rec);
}

void CapsWriter::copy_from_writer(CapsWriter* dst, const CapsWriter* src) {
  size_t i;
  size_t s = src->members.size();
  Member* m;

  for (i = 0; i < s; ++i) {
    m = src->members[i];
    switch (m->type()) {
      case 'i':
        dst->write(static_cast<IntegerMember*>(m)->value);
        break;
      case 'l':
        dst->write(static_cast<LongMember*>(m)->value);
        break;
      case 'f':
        dst->write(static_cast<FloatMember*>(m)->value);
        break;
      case 'd':
        dst->write(static_cast<DoubleMember*>(m)->value);
        break;
      case 'S':
        dst->write(static_cast<StringMember*>(m)->value.c_str());
        break;
      case 'B':
        dst->write(static_cast<BinaryMember*>(m)->value.data(),
            static_cast<BinaryMember*>(m)->value.length());
        break;
      case 'O':
        dst->write(static_cast<ObjectMember*>(m)->value);
        break;
      case 'V':
        dst->write();
        break;
    }
  }
}

CapsWriter& CapsWriter::operator = (const Caps& o) {
  if (o.type() == CAPS_TYPE_WRITER)
    copy_from_writer(this, static_cast<const CapsWriter*>(&o));
  else
    copy_from_reader(this, static_cast<const CapsReader*>(&o));
  return *this;
}

uint32_t CapsWriter::size() const {
  return members.size();
}

} // namespace rokid
