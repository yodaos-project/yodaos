#include <stdio.h>
#include "heap-sort.h"

class Foo {
public:
  uint16_t idx;
  const char* key;
};

class FooOperator {
public:
  uint16_t getIndex(const Foo &o) const {
    return o.idx;
  }
  void setIndex(Foo &o, uint16_t v) {
    o.idx = v;
  }
  bool comp(const Foo &l, const Foo &r) const {
    if (l.key == nullptr)
      return true;
    if (r.key == nullptr)
      return false;
    return strcmp(l.key, r.key) < 0;
  }
};

int main(int argc, char** argv) {
  Foo fooArray[] = {
    { 0, "hello" },
    { 0, "world" },
    { 0, nullptr },
    { 0, "a" },
    { 0, "misc" },
    { 0, "foo" },
    { 0, nullptr },
    { 0, nullptr },
    { 0, "what the fuck!!!!" },
    { 0, "a short string" },
    { 0, "a long string..............................................................................." },
    { 0, "kkaabb" },
    { 0, "shell edit" },
    { 0, "terminate" },
    { 0, "insert" },
    { 0, nullptr },
    { 0, "dhost-build" },
    { 0, "misc" },
    { 0, "57%" },
    { 0, "battery 57%" },
    { 0, "terminate" },
    { 0, "wifi ROKID.TC" },
    { 0, "dingding" },
    { 0, "air drop" },
    { 0, nullptr },
    { 0, "WeChat" },
    { 0, "open VPN connect" },
    { 0, "Macbook-Pro" },
    { 0, "dingding" },
    { 0, "terminate" },
    { 0, "foo" },
    { 0, "world" }
  };
  uint32_t size = sizeof(fooArray) / sizeof(Foo);
  FooOperator fooOp;
  HeapSort<Foo, FooOperator> sort(fooArray, size, fooOp);
  sort.sort();

  uint32_t i;
  for (i = 0; i < size; ++i) {
    printf("%u: %s\n", i, fooArray[fooArray[i].idx].key);
  }
  return 0;
}
