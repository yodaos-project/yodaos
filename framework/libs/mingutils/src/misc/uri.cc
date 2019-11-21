#include <string.h>
#include <stdlib.h>
#include "uri.h"

using namespace std;

namespace rokid {

static bool parse_scheme(Uri* uri, const char* s, int32_t& b, int32_t e) {
  int32_t i = b;

  while (i < e) {
    if (s[i] == ':') {
      if (i > b) {
        uri->scheme.assign(s, i - b);
        b = i + 1;
        return true;
      }
      break;
    }
    ++i;
  }
  return false;
}

static bool parse_authority(Uri* uri, const char* s, int32_t& b, int32_t e) {
  if (e - b < 2)
    return true;
  if (s[b] != '/' || s[b + 1] != '/')
    return true;
  int32_t nb = b + 2;
  int32_t i = nb;
  bool found_at = false;
  while (i < e) {
    if (!found_at) {
      if (s[i] == '@') {
        if (i - nb == 0) {
          // invalid uri schema://@...
          return false;
        }
        uri->user.assign(s + nb, i - nb);
        found_at = true;
        nb = i + 1;
        ++i;
        continue;
      }
    }
    if (s[i] == ':') {
      if (i - nb == 0) {
        // invalid uri schema://:
        //             schema://[user]@:
        return false;
      }
      uri->host.assign(s + nb, i - nb);
      nb = i + 1;
      // found ':', don't need find '@'
      found_at = true;
      ++i;
      continue;
    }
    if (s[i] == '/') {
      if (i - nb == 0) {
        // invalid uri schema:///
        //             schema://[user]@/
        //             schema:// [user]@host:/
        return false;
      }
      if (uri->host.empty()) {
        uri->host.assign(s + nb, i - nb);
      } else {
        string portstr(s + nb, i - nb);
        char* ep;
        uri->port = strtol(portstr.c_str(), &ep, 10);
        if (ep[0] != '\0')
          return false;
      }
      b = i;
      return true;
    }
    ++i;
  }
  return false;
}

static bool parse_path(Uri* uri, const char* s, int32_t b, int32_t e) {
  if (e - b == 0)
    return false;
  int32_t nb = b;
  int32_t i = nb;
  int32_t found_symbol = 0;
  while (i < e) {
    if (found_symbol < 1) {
      if (s[i] == '?') {
        if (i - nb == 0) {
          // invalid uri schema://[[user]@host[:port]]?
          return false;
        }
        uri->path.assign(s + nb, i - nb);
        found_symbol = 1;
        nb = i + 1;
        ++i;
        continue;
      }
    }
    if (found_symbol < 2) {
      if (s[i] == '#') {
        if (i - nb > 0) {
          if (found_symbol == 0)
            uri->path.assign(s + nb, i - nb);
          else
            uri->query.assign(s + nb, i - nb);
        }
        nb = i + 1;
        // found '#', don't need find '?'
        found_symbol = 2;
        ++i;
        continue;
      }
    }
    ++i;
  }
  if (found_symbol == 0)
    uri->path.assign(s + nb, e - nb);
  else if (found_symbol == 1)
    uri->query.assign(s + nb, e - nb);
  else
    uri->fragment.assign(s + nb, e - nb);
  return true;
}

bool Uri::parse(const char* uri) {
  if (uri == nullptr)
    return false;
  clear();

  int32_t e = strlen(uri);
  int32_t b = 0;
  if (!parse_scheme(this, uri, b, e))
    return false;
  if (!parse_authority(this, uri, b, e))
    return false;
  return parse_path(this, uri, b, e);
}

void Uri::clear() {
  scheme.clear();
  user.clear();
  host.clear();
  port = 0;
  path.clear();
  query.clear();
  fragment.clear();
}

} // namespace rokid
