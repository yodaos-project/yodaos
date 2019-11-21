#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("USAGE: %s PORT\n", argv[0]);
    return 1;
  }

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;
  struct sockaddr_in addr;
  struct hostent* hp;
  const char* host = "localhost";
  int32_t port = atoi(argv[1]);
  if (argc >= 2) {
    int n = atoi(argv[1]);
    if (n > 0)
      port = n;
  }
  hp = gethostbyname(host);
  if (hp == NULL) {
    printf("gethostbyname failed for host %s: %s\n", host, strerror(errno));
    close(fd);
    return -1;
  }
  printf("connect to %s:%d\n", host, port);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, hp->h_addr_list[0], sizeof(addr.sin_addr));
  addr.sin_port = htons(port);
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    printf("socket connect failed: %s\n", strerror(errno));
    return -1;
  }

  ssize_t c;
  char buf[256];
  while (true) {
    c = read(fd, buf, sizeof(buf) - 1);
    if (c <= 0)
      break;
    buf[c] = '\0';
    fprintf(stdout, "%s", buf);
    fflush(stdout);
  }
  close(fd);
  return 0;
}
