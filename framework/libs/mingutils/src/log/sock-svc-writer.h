#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <list>
#include <mutex>
#include <thread>
#include <chrono>
#include "fd-writer.h"
#include "uri.h"

using namespace std;
using namespace rokid;

class SocketServiceWriter : public RLogWriter {
public:
  bool init(void* arg) {
    if (arg == nullptr)
      return false;
    char *uri = reinterpret_cast<char *>(arg);
    Uri urip;
    if (!urip.parse(uri))
      return false;
    if (urip.scheme == "unix") {
      if (!init_socket(urip.path))
        return false;
    } else if (urip.scheme == "tcp") {
      if (!init_socket(urip.host, urip.port))
        return false;
    } else
      return false;
    listen_thread = thread([this]() { this->listen_routine(); });
    return true;
  }

  void destroy() {
    if (listen_thread.joinable()) {
      int fd = listen_fd;
      listen_fd = -1;
      ::shutdown(fd, SHUT_RDWR);
      ::close(fd);
      listen_thread.join();
    }
  }

  bool write(const char *data, uint32_t size) {
    lock_guard<mutex> locker(sockets_mutex);
    if (size == 0)
      return true;
    auto it = cli_sockets.begin();
    ssize_t r;
    while (it != cli_sockets.end()) {
      r = ::write(*it, data, size);
      if (r <= 0) {
        auto dit = it;
        ++it;
        ::close(*dit);
        cli_sockets.erase(dit);
        continue;
      }
      ++it;
    }
    return true;
  }

private:
  void listen_routine() {
    int newfd;

    while (listen_fd >= 0) {
      newfd = (this->*accept_func)(listen_fd);
      if (newfd < 0) {
        printf("socket accept failed: %s\n", strerror(errno));
        break;
      }
      set_write_timeout(newfd, write_timeout);
      sockets_mutex.lock();
      cli_sockets.push_back(newfd);
      sockets_mutex.unlock();
    }

    sockets_mutex.lock();
    auto it = cli_sockets.begin();
    while (it != cli_sockets.end()) {
      ::close(*it);
      ++it;
    }
    cli_sockets.clear();
    sockets_mutex.unlock();
  }

  bool init_socket(const std::string& host, int32_t port) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0)
      return false;
    int v = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
    struct sockaddr_in addr;
    struct hostent* hp;
    hp = gethostbyname(host.c_str());
    if (hp == nullptr) {
      printf("gethostbyname failed for host %s: %s\n", host.c_str(),
          strerror(errno));
      ::close(fd);
      return false;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, hp->h_addr_list[0], sizeof(addr.sin_addr));
    addr.sin_port = htons(port);
    if (::bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
      ::close(fd);
      printf("socket bind failed: %s\n", strerror(errno));
      return false;
    }
    listen(fd, 10);
    listen_fd = fd;
    accept_func = &SocketServiceWriter::tcp_accept;
    return true;
  }

  bool init_socket(const std::string& path) {
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
      return false;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path.c_str());
    unlink(path.c_str());
    if (::bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
      ::close(fd);
      printf("socket bind failed: %s\n", strerror(errno));
      return false;
    }
    listen(fd, 10);
    listen_fd = fd;
    accept_func = &SocketServiceWriter::unix_accept;
    return true;
  }

  int unix_accept(int lfd) {
    sockaddr_un addr;
    socklen_t addr_len = sizeof(addr);
    return accept(lfd, (sockaddr *)&addr, &addr_len);
  }

  int tcp_accept(int lfd) {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    return accept(lfd, (sockaddr *)&addr, &addr_len);
  }

  static void set_write_timeout(int sock, uint32_t timeout) {
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  }

private:
  int listen_fd = -1;
  std::list<int> cli_sockets;
  std::mutex sockets_mutex;
  std::thread listen_thread;
  int (SocketServiceWriter::*accept_func)(int) = nullptr;
  uint32_t write_timeout{800};
};
