#include <stdio.h>
#include <unistd.h>
#include "rlog.h"

#define TAG "rlog_demo"

int main(int argc, char** argv) {
  KLOGE(TAG, "hello world");
  KLOGW(TAG, "hello world");
  KLOGI(TAG, "hello world");
  KLOGD(TAG, "hello world");
  KLOGV(TAG, "hello world");

  RLog::add_endpoint("socket", ROKID_LOGWRITER_SOCKET);
  RLog::enable_endpoint("socket", (void *)"tcp://0.0.0.0:7777/", true);

  printf("press enter key to send tcp socket log\n");
  getchar();

  int32_t i;
  for (i = 0; i < 100; ++i) {
    KLOGE(TAG, "hello world %d", i);
    KLOGW(TAG, "hello world %d", i);
    KLOGI(TAG, "hello world %d", i);
    KLOGD(TAG, "hello world %d", i);
    KLOGV(TAG, "hello world %d", i);
    sleep(1);
  }

  return 0;
}
