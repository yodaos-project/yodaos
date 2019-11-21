#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"

using namespace std;

namespace rokid {

// ======================HttpRequest=========================

void HttpRequest::setMethod(uint32_t method) {
  this->method = method;
}

void HttpRequest::setPath(const char* path) {
  this->path = path;
}

void HttpRequest::addHeaderField(const char* key, const char* value) {
  if (key == nullptr || key[0] == '\0')
    return;
  headerFields.emplace_back();
  headerFields.back().key = key;
  headerFields.back().value = value;
}

static const char* methodString(uint32_t method) {
  static const char* mstr[] = {
    "GET",
    "POST"
  };
  if (method < sizeof(mstr) / sizeof(intptr_t))
    return mstr[method];
  return mstr[0];
}

int32_t HttpRequest::build(char* buf, uint32_t bufsize, const char* body) {
  uint32_t total = 0;
  int c;

  c = snprintf(buf, bufsize, "%s %s HTTP/1.1\r\n",
      methodString(method), path == nullptr ? "/" : path);
  if (c < 0)
    return 0;
  total += c;
  if (total >= bufsize)
    return 0;
  size_t i;
  size_t numFields = headerFields.size();
  for (i = 0; i < numFields; ++i) {
    c = snprintf(buf + total, bufsize - total, "%s: %s\r\n",
        headerFields[i].key, headerFields[i].value.c_str());
    if (c < 0)
      return 0;
    total += c;
    if (total >= bufsize)
      return 0;
  }
  if (body == nullptr) {
    body = "";
  }
  c = snprintf(buf + total, bufsize - total, "\r\n%s", body);
  if (c < 0)
    return 0;
  total += c;
  if (total >= bufsize)
    return 0;
  return total;
}

// ======================HttpResponse=========================

int32_t HttpResponse::parse(const char* str, uint32_t len) {
  if (len == 0)
    return NOT_FINISH;
  if (str == nullptr)
    return INVALID;

  // parse headers
  const char* lb = str;
  const char* le = str;
  const char* strend = str + len;
  vector<string> lines;
  while (le < strend) {
    if (*le == '\r') {
      if (le + 1 == strend)
        return NOT_FINISH;
      if (le[1] == '\n') {
        lines.emplace_back(lb, le - lb);
        lb = le + 2;
        le = lb;
        // an empty line, end of headers
        if (lines.back().length() == 0)
          break;
        continue;
      } else {
        return INVALID;
      }
    }
    ++le;
  }
  if (le > lb)
    return NOT_FINISH;
  if (lines.back().length())
    return NOT_FINISH;
  // only one empty line
  if (lines.size() == 1)
    return INVALID;
  if (!parseStatusLine(lines[0]))
    return INVALID;
  if (!parseHeaderFields(++lines.begin(), --lines.end()))
    return INVALID;

  // parse body
  map<string, string>::iterator mit = headerFields.find("Content-Length");
  if (mit != headerFields.end()) {
    char* ep;
    if (mit->second.length() == 0)
      return INVALID;
    long l = strtol(mit->second.c_str(), &ep, 10);
    if (ep[0] != '\0' || l < 0)
      return INVALID;
    contentLength = l;
  }
  uint32_t remain = strend - lb;
  if (remain >= contentLength) {
    body.assign(lb, contentLength);
    return (lb - str) + contentLength;
  }
  body.assign(lb, remain);
  return BODY_NOT_FINISH;
}

bool HttpResponse::parseStatusLine(const string& line) {
  size_t n1, n2;

  // skip http version
  n1 = line.find(' ');
  if (n1 == string::npos)
    return false;

  // parse status code
  n2 = line.find(' ', n1 + 1);
  if (n2 == string::npos || n2 - n1 > 4)
    return false;
  memcpy(statusCode, line.data() + n1 + 1, 3);
  statusCode[3] = '\0';

  reason.assign(line, n2 + 1, string::npos);
  return true;
}

static void trim(const string& str, size_t begin, size_t end, string& res) {
  const char* b;
  const char* e;

  b = str.data() + begin;
  e = str.data() + end;
  while (b < e) {
    if (*b != ' ')
      break;
    ++b;
  }
  while (b < e) {
    --e;
    if (*e != ' ') {
      ++e;
      break;
    }
  }
  res.assign(b, e - b);
}

bool HttpResponse::parseHeaderFields(vector<string>::iterator& begin,
    vector<string>::iterator& end) {
  vector<string>::iterator it;
  size_t n;
  string key;
  string val;

  for (it = begin; it != end; ++it) {
    n = (*it).find(':');
    if (n == string::npos)
      goto failed;
    trim((*it), 0, n, key);
    trim((*it), n + 1, (*it).length(), val);
    if (key.length() == 0 || val.length() == 0)
      goto failed;
    if (headerFields.find(key) != headerFields.end())
      goto failed;
    headerFields[key] = val;
  }
  return true;

failed:
  headerFields.clear();
  return false;
}

} // namespace rokid
