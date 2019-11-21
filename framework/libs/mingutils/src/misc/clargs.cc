#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <cctype>
#include <vector>
#include "clargs.h"
#include "merge-sort.h"

static const uint32_t BUFFER_LENGTH = 1024 * 1024;

using namespace std;

class CLPairRaw {
public:
  uint64_t idx:16;
  uint64_t key:24;
  uint64_t value:24;
};

class StringPool {
public:
  StringPool() {
    buffer = (char*)mmap(nullptr, BUFFER_LENGTH, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  }

  ~StringPool() {
    munmap(buffer, BUFFER_LENGTH);
  }

  uint32_t push(const char* p, uint32_t size) {
    char* d = buffer + buf_offset;
    uint32_t r = buf_offset;
    if (size)
      memcpy(d, p, size);
    d[size] = '\0';
    buf_offset += size + 1;
    return r;
  }

  const char* get(uint32_t idx) const {
    if (idx == 0 || idx >= buf_offset)
      return nullptr;
    return buffer + idx;
  }

private:
  char* buffer;
  uint32_t buf_offset = 1;
};

class PairRawOperator {
public:
  PairRawOperator(StringPool *p) : pool(p) {
  }

  uint16_t getIndex(const CLPairRaw &o) const {
    return o.idx;
  }
  void setIndex(CLPairRaw &o, uint16_t v) {
    o.idx = v;
  }

  bool comp(const CLPairRaw &l, const CLPairRaw &r) const {
    const char* lk = pool->get(l.key);
    const char* rk = pool->get(r.key);
    if (lk == nullptr)
      return rk != nullptr;
    if (rk == nullptr)
      return false;
    return strcmp(lk, rk) < 0;
  }

private:
  StringPool *pool;
};

class clargs_inst : public CLArgs {
public:
  clargs_inst() {
    clpairs.reserve(16);
  }

  uint32_t size() const {
    return clpairs.size();
  }

  const CLPair& at(uint32_t idx, CLPair &pair) const {
    if (idx >= clpairs.size()) {
      pair.key = nullptr;
      pair.value = nullptr;
    } else {
      createPair(getRawPair(idx), pair);
    }
    return pair;
  }

  bool find(const char* key, uint32_t *begin, uint32_t *end) const {
    size_t sz = clpairs.size();
    size_t i;
    int32_t r;
    // 0  not found
    // 1  found begin
    int32_t stat = 0;
    CLPair pair;
    for (i = 0; i < sz; ++i) {
      r = compareCLPair(key, at(i, pair));
      if (r < 0) {
        break;
      } else if (r == 0) {
        if (stat == 0) {
          stat = 1;
          if (begin)
            *begin = i;
        }
      }
    }
    if (stat == 1 && end) {
      *end = i - 1;
    }
    return stat != 0;
  }

  bool parse(int32_t argc, char** argv) {
    int32_t i;
    size_t len;
    const char* s;
    uint32_t psr;
    char prev_single_opt = '\0';

    for (i = 0; i < argc; ++i) {
      s = argv[i];
      len = strlen(s);
      if (len > 2) {
        // --foo=bar
        // --foo
        if (s[0] == '-' && s[1] == '-') {
          if (parse_pair(s + 2, len - 2)) {
            prev_single_opt = '\0';
            continue;
          }
          goto parse_arg;
        }
      }
      if (len > 1) {
        if (s[0] == '-') {
          // 检测到单个'-'加上单个字母或数字
          // 记录在prev_single_opt变量中
          // 否则清空prev_single_opt变量
          // -abc
          // -a xxoo
          // -b -c
          psr = parse_single_minus(s + 1, len - 1);
          if (psr > 0) {
            prev_single_opt = psr == 1 ? s[1] : '\0';
            continue;
          }
        }
      }

parse_arg:
      if (prev_single_opt) {
        uint32_t val = strpool.push(s, len);
        set_last_pair_value(val);
        prev_single_opt = '\0';
      } else {
        uint32_t val = strpool.push(s, len);
        add_pair(0, val);
      }
    }
    PairRawOperator op(&strpool);
    MergeSort<CLPairRaw, uint16_t, PairRawOperator> mergesort(clpairs.data(), clpairs.size(), op);
    mergesort.sort();
    return true;
  }

private:
  bool parse_pair(const char* s, size_t l) {
    if (!isalpha(s[0]))
      return false;
    size_t i;
    uint32_t key = 0;
    uint32_t val = 0;
    for (i = 1; i < l; ++i) {
      if (s[i] == '=') {
        key = strpool.push(s, i);
        val = strpool.push(s + i + 1, l - i - 1);
        break;
      }
    }
    if (i == l)
      key = strpool.push(s, l);
    if (l > i + 1) {
      // '=' 后面第一个字符不能又是'='
      if (s[i + 1] == '=')
        return false;
    }
    add_pair(key, val);
    return true;
  }

  uint32_t parse_single_minus(const char* s, size_t l) {
    size_t i;
    for (i = 0; i < l; ++i) {
      if (!isalnum(s[i]))
        return 0;
    }
    uint32_t key;
    for (i = 0; i < l; ++i) {
      key = strpool.push(s + i, 1);
      add_pair(key, 0);
    }
    return l;
  }

  void add_pair(uint32_t k, uint32_t v) {
    CLPairRaw t;
    t.idx = 0;
    t.key = k;
    t.value = v;
    clpairs.push_back(t);
  }

  void set_last_pair_value(uint32_t v) {
    clpairs.back().value = v;
  }

  const CLPairRaw &getRawPair(uint32_t idx) const {
    return clpairs[clpairs[idx].idx];
  }

  CLPair &createPair(const CLPairRaw &raw, CLPair &pair) const {
    pair.key = strpool.get(raw.key);
    pair.value = strpool.get(raw.value);
    return pair;
  }

  int32_t compareCLPair(const char *key, const CLPair &pair) const {
    if (key == nullptr) {
      return pair.key == nullptr ? 0 : -1;
    }
    if (pair.key == nullptr)
      return 1;
    return strcmp(key, pair.key);
  }

private:
  vector<CLPairRaw> clpairs;
  StringPool strpool;
};

shared_ptr<CLArgs> CLArgs::parse(int32_t argc, char** argv) {
  shared_ptr<clargs_inst> r = make_shared<clargs_inst>();
  if (!r->parse(argc, argv))
    return nullptr;
  return static_pointer_cast<CLArgs>(r);
}

bool CLPair::match(const char* key) const {
  if (key == nullptr && this->key == nullptr)
    return true;
  if (this->key == nullptr || key == nullptr)
    return false;
  return strcmp(key, this->key) == 0;
}

bool CLPair::to_integer(int32_t& res) const {
  if (value == nullptr)
    return false;
  char* ep;
  res = strtol(value, &ep, 10);
  return ep != value && ep[0] == '\0';
}

clargs_h clargs_parse(int32_t argc, char** argv) {
  clargs_inst* r = new clargs_inst();
  if (!r->parse(argc, argv)) {
    delete r;
    return 0;
  }
  return reinterpret_cast<intptr_t>(r);
}

void clargs_destroy(clargs_h handle) {
  if (handle == 0)
    return;
  delete reinterpret_cast<clargs_inst*>(handle);
}

uint32_t clargs_size(clargs_h handle) {
  return reinterpret_cast<clargs_inst*>(handle)->size();
}

int32_t clargs_get(clargs_h handle, uint32_t idx, const char** key, const char** value) {
  clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
  if (idx >= inst->size())
    return -1;
  CLPair pair;
  inst->at(idx, pair);
  *key = pair.key;
  *value = pair.value;
  return 0;
}

int32_t clargs_get_integer(clargs_h handle, uint32_t idx, const char** key, int32_t* res) {
  clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
  if (idx >= inst->size())
    return -1;
  CLPair pair;
  inst->at(idx, pair);
  *key = pair.key;
  if (res == nullptr)
    return -1;
  if (!pair.to_integer(*res))
    return -2;
  return 0;
}
