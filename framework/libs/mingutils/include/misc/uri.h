#pragma once

#include <stdint.h>
#include <string>

namespace rokid {

class Uri {
public:
  bool parse(const char* uri);

  void clear();

public:
  std::string scheme;
  std::string user;
  std::string host;
  int32_t port = 0;
  std::string path;
  std::string query;
  std::string fragment;
};

} // namespace rokid
