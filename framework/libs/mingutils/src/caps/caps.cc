#include <arpa/inet.h>
#include <string>
#include "caps.h"
#include "reader.h"
#include "writer.h"

using namespace std;
using namespace rokid;

namespace rokid {

char CAPS_MAGIC[4] = { 0x1e, 'A', 'P', CAPS_VERSION };

bool check_version(uint8_t version) {
  return version > 2 && version <= CAPS_VERSION;
}

int32_t check_header(const Header* header, uint32_t& length) {
  if (header->magic[0] & CAPS_FLAG_NET_BYTEORDER)
    length = ntohl(header->length);
  else
    length = header->length;
  if ((header->magic[0] & CAPS_MAGIC_MASK) != CAPS_MAGIC[0])
    return CAPS_ERR_CORRUPTED;
  if (header->magic[1] != CAPS_MAGIC[1]
      || header->magic[2] != CAPS_MAGIC[2])
    return CAPS_ERR_CORRUPTED;
  if (!check_version(header->magic[3]))
    return CAPS_ERR_VERSION_UNSUPP;
  return CAPS_SUCCESS;
}

} // namespace rokid

shared_ptr<Caps> Caps::new_instance() {
  return make_shared<CapsWriter>();
}

int32_t Caps::parse(const void* data, uint32_t length,
    shared_ptr<Caps>& caps, bool duplicate) {
  if (data == nullptr || length == 0)
    return CAPS_ERR_INVAL;
  shared_ptr<CapsReader> r = make_shared<CapsReader>();
  int32_t code = r->parse(data, length, duplicate);
  if (code != CAPS_SUCCESS)
    return code;
  caps = static_pointer_cast<Caps>(r);
  return CAPS_SUCCESS;
}

shared_ptr<Caps> Caps::convert(caps_t caps) {
  shared_ptr<Caps> r;
  if (caps) {
    Caps* b = reinterpret_cast<Caps*>(caps);
    if (b->type() == CAPS_TYPE_WRITER) {
      CapsWriter* w = new CapsWriter();
      *w = *b;
      r.reset(w);
    } else {
      CapsReader* rd = new CapsReader();
      *rd = *b;
      r.reset(rd);
    }
  }
  return r;
}

caps_t Caps::convert(std::shared_ptr<Caps>& caps) {
  if (caps.get() == nullptr)
    return 0;
  if (caps->type() == CAPS_TYPE_WRITER) {
    CapsWriter* w = new CapsWriter();
    *w = *caps.get();
    return reinterpret_cast<caps_t>(w);
  }
  CapsReader* r = new CapsReader();
  *r = *caps.get();
  return reinterpret_cast<caps_t>(r);
}

int32_t Caps::binary_info(const void* data, uint32_t* version,
    uint32_t* length) {
  if (data == nullptr)
    return CAPS_ERR_INVAL;
  const Header* h = reinterpret_cast<const Header*>(data);
  uint32_t data_length;
  int32_t r = check_header(h, data_length);
  if (r)
    return r;
  if (version)
    *version = h->magic[3];
  if (length)
    *length = data_length;
  return CAPS_SUCCESS;
}

caps_t caps_create() {
  return reinterpret_cast<caps_t>(new CapsWriter());
}

int32_t caps_parse(const void* data, uint32_t length, caps_t* result) {
  if (data == nullptr || length == 0 || result == nullptr)
    return CAPS_ERR_INVAL;
  CapsReader* reader = new CapsReader();
  int32_t r = reader->parse(data, length, true);
  if (r != CAPS_SUCCESS)
    return r;
  *result = reinterpret_cast<caps_t>(reader);
  return CAPS_SUCCESS;
}

int32_t caps_serialize(caps_t caps, void* buf, uint32_t bufsize) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  Caps* writer = reinterpret_cast<Caps*>(caps);
  if (writer->type() != CAPS_TYPE_WRITER)
    return CAPS_ERR_RDONLY;
  return static_cast<CapsWriter*>(writer)->serialize(buf, bufsize,
      CAPS_FLAG_NET_BYTEORDER);
}

int32_t caps_write_integer(caps_t caps, int32_t v) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->write(v);
}

int32_t caps_write_float(caps_t caps, float v) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->write(v);
}

int32_t caps_write_long(caps_t caps, int64_t v) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->write(v);
}

int32_t caps_write_double(caps_t caps, double v) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->write(v);
}

int32_t caps_write_string(caps_t caps, const char* v) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->write(v);
}

int32_t caps_write_binary(caps_t caps, const void* v, uint32_t length) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->write(v, length);
}

int32_t caps_write_object(caps_t caps, caps_t v) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  Caps* writer = reinterpret_cast<Caps*>(caps);
  if (writer->type() != CAPS_TYPE_WRITER)
    return CAPS_ERR_RDONLY;
  Caps* o = reinterpret_cast<Caps*>(v);
  int8_t* data = nullptr;
  int32_t size;
  bool allocated = false;
  if (o == nullptr) {
    size = 0;
  } else if (o->type() == CAPS_TYPE_WRITER) {
    size = o->serialize(nullptr, 0);
    if (size > 0) {
      data = new int8_t[size];
      o->serialize(data, size);
      allocated = true;
    } else {
      size = 0;
    }
  } else {
    size = static_cast<CapsReader*>(o)->binary_size();
    data = (int8_t*)static_cast<CapsReader*>(o)->binary_data();
  }
  static_cast<CapsWriter*>(writer)->write(data, size);
  if (allocated)
    delete[] data;
  return CAPS_SUCCESS;
}

int32_t caps_write_void(caps_t caps) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->write();
}

int32_t caps_read_integer(caps_t caps, int32_t* r) {
  if (caps == 0 || r == nullptr)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->read(*r);
}

int32_t caps_read_long(caps_t caps, int64_t* r) {
  if (caps == 0 || r == nullptr)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->read(*r);
}

int32_t caps_read_float(caps_t caps, float* r) {
  if (caps == 0 || r == nullptr)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->read(*r);
}

int32_t caps_read_double(caps_t caps, double* r) {
  if (caps == 0 || r == nullptr)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->read(*r);
}

int32_t caps_read_string(caps_t caps, const char** r) {
  if (caps == 0 || r == nullptr)
    return CAPS_ERR_INVAL;
  Caps* reader = reinterpret_cast<Caps*>(caps);
  if (reader->type() != CAPS_TYPE_READER)
    return CAPS_ERR_WRONLY;
  return static_cast<CapsReader*>(reader)->read(*r);
}

int32_t caps_read_binary(caps_t caps, const void** r, uint32_t* length) {
  if (caps == 0 || r == nullptr || length == nullptr)
    return CAPS_ERR_INVAL;
  Caps* reader = reinterpret_cast<Caps*>(caps);
  if (reader->type() != CAPS_TYPE_READER)
    return CAPS_ERR_WRONLY;
  return static_cast<CapsReader*>(reader)->read(*r, *length);
}

int32_t caps_read_object(caps_t caps, caps_t* r) {
  if (caps == 0 || r == nullptr)
    return CAPS_ERR_INVAL;
  Caps* reader = reinterpret_cast<Caps*>(caps);
  if (reader->type() != CAPS_TYPE_READER)
    return CAPS_ERR_WRONLY;
  string bin;
  int32_t code = static_cast<CapsReader*>(reader)->read_binary(bin);
  if (code != CAPS_SUCCESS)
    return code;
  if (bin.length() == 0) {
    *r = 0;
    return CAPS_SUCCESS;
  }
  CapsReader* sub = new CapsReader();
  code = sub->parse(bin.data(), bin.length(), true);
  if (code != CAPS_SUCCESS) {
    delete sub;
    return code;
  }
  *r = reinterpret_cast<caps_t>(sub);
  return CAPS_SUCCESS;
}

int32_t caps_read_void(caps_t caps) {
  if (caps == 0)
    return CAPS_ERR_INVAL;
  return reinterpret_cast<Caps*>(caps)->read();
}

void caps_destroy(caps_t caps) {
  if (caps)
    delete reinterpret_cast<Caps*>(caps);
}

int32_t caps_binary_info(const void* data, uint32_t* version,
    uint32_t* length) {
  return Caps::binary_info(data, version, length);
}

/**
caps_t caps_duplicate(caps_t src, uint32_t mode) {
  if (src == 0)
    return 0;
  caps_t res = 0;
  if (mode == CAPS_DUP_RDONLY) {
    CapsReader* r = new CapsReader();
    *r = *reinterpret_cast<Caps*>(src);
    res = reinterpret_cast<caps_t>(r);
  } else if (mode == CAPS_DUP_WRONLY) {
    CapsWriter* w = new CapsWriter();
    *w = *reinterpret_cast<Caps*>(src);
    res = reinterpret_cast<caps_t>(w);
  }
  return res;
}
*/
