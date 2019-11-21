#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <map>

namespace rokid {

class HttpRequest {
public:
  void setMethod(uint32_t method);

  void setPath(const char* path);

  void addHeaderField(const char* key, const char* value);

  int32_t build(char* buf, uint32_t bufsize, const char* body = nullptr);

public:
  static const uint32_t METHOD_GET = 0;
  static const uint32_t METHOD_POST = 1;

private:
  typedef struct {
    const char* key;
    std::string value;
  } HeaderField;

  uint32_t method = METHOD_GET;
  const char* path = nullptr;
  std::vector<HeaderField> headerFields;
};

class HttpResponse {
public:
  // return response length if success
  // return error code if failed
  int32_t parse(const char* str, uint32_t len);

private:
  bool parseStatusLine(const std::string& line);

  bool parseHeaderFields(std::vector<std::string>::iterator& begin,
      std::vector<std::string>::iterator& end);

public:
  char statusCode[4] = {0};
  std::string reason;
  std::map<std::string, std::string> headerFields;
  uint32_t contentLength = 0;
  std::string body;

  static const int32_t NOT_FINISH = -1;
  static const int32_t BODY_NOT_FINISH = -2;
  static const int32_t INVALID = -3;
};

} // namespace rokid
