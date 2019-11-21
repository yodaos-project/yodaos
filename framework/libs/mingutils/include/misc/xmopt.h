#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <map>
#include <vector>
#include "strpool.h"
#include "merge-sort.h"

#define XMOPT_ERR_UNEXCEPT -1

class XMOptions {
private:
  class OptionDecl {
  public:
    std::string abbKey;
    std::string fullKey;
    std::string desc;
  };
  typedef std::list<OptionDecl> OptDeclList;
  typedef std::map<std::string, OptionDecl*> OptDeclMap;

public:
  class Option {
  public:
    Option(StringPool& sp) : strpool(sp) {
      _idx = 0xffff;
      _key = 0;
      _value = 0;
    }

    const char* value() const {
      return strpool.get(_value);
    }

    int32_t integer() const {
      auto s = strpool.get(_value);
      if (s == nullptr)
        return 0;
      return (int32_t)strtol(s, nullptr, 10);
    }

    const char* key() const {
      return strpool.get(_key);
    }

  public:
    uint64_t _idx:16;
    uint64_t _key:24;
    uint64_t _value:24;

  private:
    StringPool& strpool;
  };
  typedef std::vector<Option> OptionArray;

  class OptionSet {
  public:
    OptionSet(OptionArray& opts, StringPool& sp) : options(&opts), strpool(&sp) {
    }

    Option* next() {
      if (abbIter >= 0) {
        auto opt = options->data() + options->at(abbIter)._idx;
        ++abbIter;
        if (!isNextOptionValid(opt, abbIter))
          abbIter = -1;
        return opt;
      }
      if (fullIter >= 0) {
        auto opt = options->data() + options->at(fullIter)._idx;
        ++fullIter;
        if (!isNextOptionValid(opt, fullIter))
          fullIter = -1;
        return opt;
      }
      return nullptr;
    }

    void setFirstAbb(int16_t idx) {
      abbIter = idx;
    }

    void setFirstFull(int16_t idx) {
      fullIter = idx;
    }

    bool empty() {
      return abbIter < 0 && fullIter < 0;
    }

  private:
    bool isNextOptionValid(Option* curOpt, int16_t nextIdx) {
      if (nextIdx >= options->size())
        return false;
      auto nextOpt = options->data() + options->at(nextIdx)._idx;
      if (curOpt->_key == 0)
        return nextOpt->_key == 0;
      auto k1 = strpool->get(curOpt->_key);
      auto k2 = strpool->get(nextOpt->_key);
      return strcmp(k1, k2) == 0;
    }

  private:
    int16_t abbIter{-1};
    int16_t fullIter{-1};
    OptionArray* options;
    StringPool* strpool;
  };

private:
  class OptionSortOperator {
  public:
    OptionSortOperator(StringPool* p) : pool(p) {
    }

    uint16_t getIndex(const Option& o) const {
      return o._idx;
    }
    void setIndex(Option& o, uint16_t v) {
      o._idx = v;
    }

    bool comp(const Option& l, const Option& r) const {
      const char* lk = pool->get(l._key);
      const char* rk = pool->get(r._key);
      if (lk == nullptr)
        return rk != nullptr;
      if (rk == nullptr)
        return false;
      return strcmp(lk, rk) < 0;
    }

  private:
    StringPool *pool;
  };

  class Error {
  public:
    int32_t code{0};
    std::string arg;
  };

public:
  void option(const char* abbKey, const char* fullKey, const char* desc) {
    auto it = optionDeclarations.emplace(optionDeclarations.end());
    if (abbKey)
      it->abbKey = abbKey;
    if (fullKey)
      it->fullKey = fullKey;
    if (desc)
      it->desc = desc;
    if (!it->abbKey.empty()) {
      auto r = optionDeclMap.insert(std::make_pair(it->abbKey.c_str(), &(*it)));
      if (!r.second) {
        optionDeclarations.erase(it);
        return;
      }
    }
    if (!it->fullKey.empty()) {
      auto r = optionDeclMap.insert(std::make_pair(it->fullKey.c_str(), &(*it)));
      if (!r.second) {
        optionDeclMap.erase(it->abbKey.c_str());
        optionDeclarations.erase(it);
        return;
      }
    }
  }

  bool parse(int argc, char** argv) {
    int32_t i;
    size_t len;
    const char* s;
    uint32_t psr;
    char prevSingleOpt = '\0';

    for (i = 0; i < argc; ++i) {
      s = argv[i];
      len = strlen(s);
      if (len > 2) {
        // --foo=bar
        // --foo
        if (s[0] == '-' && s[1] == '-') {
          if (parsePair(s + 2, len - 2)) {
            prevSingleOpt = '\0';
            continue;
          }
          goto parseArg;
        }
      }
      if (len > 1) {
        if (s[0] == '-') {
          // 检测到单个'-'加上单个字母或数字
          // 记录在prevSingleOpt变量中
          // 否则清空prevSingleOpt变量
          // -abc
          // -a xxoo
          // -b -c
          psr = parseSingleMinus(s + 1, len - 1);
          if (psr > 0) {
            prevSingleOpt = psr == 1 ? s[1] : '\0';
            continue;
          }
        }
      }

parseArg:
      if (prevSingleOpt) {
        uint32_t val = strpool.push(s, len);
        setLastOptionValue(val);
        prevSingleOpt = '\0';
      } else {
        uint32_t val = strpool.push(s, len);
        addOption(0, val);
      }
    }

    if (!checkOptions())
      return false;
    sortOptions();
    return true;
  }

  void errorMsg(std::string& msg) {
    if (err.code == XMOPT_ERR_UNEXCEPT) {
      msg = "unexcepted option \"";
      msg.append(err.arg);
      msg.append("\"");
    }
  }

  void prompt(std::string &msg) {
    msg = "options:\n";
    bool comma;
    auto it = optionDeclarations.begin();
    while (it != optionDeclarations.end()) {
      comma = false;
      msg += "  ";
      if (!it->abbKey.empty()) {
        msg += '-';
        msg += it->abbKey;
        comma = true;
      }
      if (!it->fullKey.empty()) {
        if (comma)
          msg += ',';
        msg += "--";
        msg += it->fullKey;
      }
      msg += "    ";
      msg += it->desc;
      msg += '\n';
      ++it;
    }
  }

  OptionSet find(const char* key) {
    OptionSet res(options, strpool);
    if (key == nullptr || key[0] == '\0') {
      if (!options.empty() && options[options[0]._idx]._key == 0)
        res.setFirstFull(0);
      return res;
    }

    auto it = optionDeclMap.find(key);
    if (it == optionDeclMap.end())
      return res;
    size_t sz = options.size();
    size_t i;
    bool foundAbb = false;
    bool foundFull = false;
    Option* opt;
    for (i = 0; i < sz; ++i) {
      opt = options.data() + options[i]._idx;
      if (!foundAbb) {
        if (opt->key() && it->second->abbKey == opt->key()) {
          foundAbb = true;
          res.setFirstAbb(i);
          if (foundFull)
            break;
        }
      }
      if (!foundFull) {
        if (opt->key() && it->second->fullKey == opt->key()) {
          foundFull = true;
          res.setFirstFull(i);
          if (foundAbb)
            break;
        }
      }
    }
    return res;
  }

private:
  bool parsePair(const char* s, size_t l) {
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
    addOption(key, val);
    return true;
  }

  uint32_t parseSingleMinus(const char* s, size_t l) {
    size_t i;
    for (i = 0; i < l; ++i) {
      if (!isalnum(s[i]))
        return 0;
    }
    uint32_t key;
    for (i = 0; i < l; ++i) {
      key = strpool.push(s + i, 1);
      addOption(key, 0);
    }
    return l;
  }

  void addOption(uint32_t k, uint32_t v) {
    Option t(strpool);
    t._idx = 0;
    t._key = k;
    t._value = v;
    options.push_back(t);
  }

  void setLastOptionValue(uint32_t v) {
    options.back()._value = v;
  }

  bool checkOptions() {
    size_t i;
    for (i = 0; i < options.size(); ++i) {
      auto key = strpool.get(options[i]._key);
      if (key && optionDeclMap.find(key) == optionDeclMap.end()) {
        err.code = XMOPT_ERR_UNEXCEPT;
        err.arg = key;
        return false;
      }
    }
    return true;
  }

  void sortOptions() {
    OptionSortOperator op(&strpool);
    MergeSort<Option, uint16_t, OptionSortOperator> mergesort(options.data(), options.size(), op);
    mergesort.sort();
  }

private:
  OptDeclList optionDeclarations;
  OptDeclMap optionDeclMap;
  OptionArray options;
  StringPool strpool{0x100000};
  Error err;
};
