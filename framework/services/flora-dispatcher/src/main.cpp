#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "flora-svc.h"
#include "clargs.h"
#include "rlog.h"
#include "defs.h"

#define MACRO_TO_STRING(x) MACRO_TO_STRING1(x)
#define MACRO_TO_STRING1(x) #x

using namespace flora;
using namespace std;

static void print_prompt(const char* progname) {
  static const char* prompt =
    "USAGE: %s [options]\n"
    "options:\n"
    "\t--help    打印帮助信息\n"
    "\t--version    版本号\n"
    "\t--uri=*    指定flora服务uri\n"
    "\t--msg-buf-size=*    指定flora消息缓冲区大小\n"
    "\t--log-file=*    指定log输出文件路径\n"
    "\t--log-service-port=*    指定log服务端口"
    "\t--keepalive-timeout=*   tcp连接keepalive超时时间"
    ;
  KLOGI(TAG, prompt, progname);
}

class CmdlineArgs {
public:
  CmdlineArgs() {
    uris.reserve(2);
  }

  void add_uri(const char* uri) {
    uris.emplace_back(uri);
  }

  uint32_t msg_buf_size = 0;
  vector<string> uris;
  string log_file;
  int32_t log_port = 0;
  uint32_t keepalive_timeout = 60000;
};

void run(CmdlineArgs& args);

static bool parse_cmdline(shared_ptr<CLArgs> &clargs, CmdlineArgs& res) {
  int32_t iv;
  uint32_t i;
  CLPair pair;

  for (i = 1; i < clargs->size(); ++i) {
    clargs->at(i, pair);
    if (pair.match("msg-buf-size")) {
      if (!pair.to_integer(iv))
        goto invalid_option;
      res.msg_buf_size = iv;
    } else if (pair.match("uri")) {
      if (pair.value == nullptr || pair.value[0] == '\0')
        goto invalid_option;
      res.add_uri(pair.value);
    } else if (pair.match("log-file")) {
      res.log_file = pair.value;
    } else if (pair.match("log-service-port")) {
      if (!pair.to_integer(iv))
        goto invalid_option;
      res.log_port = iv;
    } else if (pair.match("keepalive-timeout")) {
      if (!pair.to_integer(iv))
        goto invalid_option;
      res.keepalive_timeout = iv;
    } else {
      goto invalid_option;
    }
  }
  if (res.uris.empty()) {
    KLOGE(TAG, "missing service uri");
    return false;
  }
  return true;

invalid_option:
  if (pair.value)
    KLOGE(TAG, "invalid option: --%s=%s", pair.key, pair.value);
  else
    KLOGE(TAG, "invalid option: --%s", pair.key);
  return false;
}

int main(int argc, char** argv) {
  shared_ptr<CLArgs> h = CLArgs::parse(argc, argv);
  if (h == nullptr || h->find("help", nullptr, nullptr)) {
    print_prompt(argv[0]);
    return 0;
  }
  if (h->find("version", nullptr, nullptr)) {
    KLOGI(TAG, "git commit id: %s", MACRO_TO_STRING(GIT_COMMIT_ID));
    return 0;
  }
  CmdlineArgs cmdargs;
  if (!parse_cmdline(h, cmdargs)) {
    return 1;
  }
  h.reset();

  run(cmdargs);
  return 0;
}

static bool set_log_file(const std::string& file) {
  if (file.length() == 0)
    return false;
  int fd = open(file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fd < 0)
    return false;
  if (RLog::add_endpoint("file", ROKID_LOGWRITER_FD) != 0) {
    ::close(fd);
    return false;
  }
  return RLog::enable_endpoint("file", (void *)(intptr_t)fd, true) == 0;
}

static bool set_log_port(int32_t port) {
  if (port > 0) {
    char uri[32];
    snprintf(uri, sizeof(uri), "tcp://%s:%d/", "0.0.0.0", port);
    if (RLog::add_endpoint("socket", ROKID_LOGWRITER_SOCKET) != 0)
      return false;
    return RLog::enable_endpoint("socket", (void *)uri, true) == 0;
  }
  return false;
}

void run(CmdlineArgs& args) {
  bool b1 = set_log_file(args.log_file);
  bool b2 = set_log_port(args.log_port);
  if (b1 || b2)
    RLog::remove_endpoint("std");

  KLOGI(TAG, "msg buf size = %u, uri count %d", args.msg_buf_size,
      args.uris.size());
  shared_ptr<Dispatcher> disp = Dispatcher::new_instance(
      FLORA_DISP_FLAG_MONITOR, args.msg_buf_size);
  size_t i;
  vector<shared_ptr<Poll> > polls;
  polls.reserve(2);
  for (i = 0; i < args.uris.size(); ++i) {
    KLOGI(TAG, "uri = %s", args.uris[i].c_str());
    shared_ptr<Poll> tpoll = Poll::new_instance(args.uris[i].c_str());
    if (tpoll.get() == nullptr) {
      KLOGE(TAG, "create poll failed for uri %s", args.uris[i].c_str());
      return;
    }
    tpoll->config(FLORA_POLL_OPT_KEEPALIVE_TIMEOUT, args.keepalive_timeout);
    tpoll->start(disp);
    polls.push_back(tpoll);
  }
  disp->run(true);
  for (i = 0; i < polls.size(); ++i)
    polls[i]->stop();
}
