#include <mutex>
#include <condition_variable>
#include "gtest/gtest.h"
#include "circle-stream.h"
#include "thr-pool.h"

#define MAX_NUMBER 100
#define REPEAT_WRITE 100

using namespace std;
using namespace rokid;

TEST(CircleStream, randomAccess) {
  struct {
    MmapCircleStream stream;
    mutex streamMutex;
    condition_variable streamAvail;
    char data[MAX_NUMBER];
    bool testing{true};
  } testVariables;
  ThreadPool thrPool{1};
  testVariables.stream.create(4096);
  thrPool.push([&testVariables]() {
    unique_lock<mutex> locker(testVariables.streamMutex);
    uint32_t size;
    char* p;
    uint32_t offset{0};
    while (true) {
      p = (char*)testVariables.stream.peek(size);
      if (p == nullptr) {
        if (!testVariables.testing)
          break;
        testVariables.streamAvail.wait(locker);
      } else {
        for (int32_t i = 0; i < size; ++i) {
          ASSERT_EQ(offset % MAX_NUMBER, p[i]);
          ++offset;
        }
        testVariables.stream.erase(size);
      }
    }
    EXPECT_EQ(offset, MAX_NUMBER * REPEAT_WRITE);
  });

  int32_t i;
  for (i = 0; i < MAX_NUMBER; ++i) {
    testVariables.data[i] = i;
  }
  uint32_t off;
  for (i = 0; i < REPEAT_WRITE; ++i) {
    off = 0;
    while (true) {
      testVariables.streamMutex.lock();
      auto c = testVariables.stream.write(
          testVariables.data + off, MAX_NUMBER - off);
      testVariables.streamAvail.notify_one();
      off += c;
      testVariables.streamMutex.unlock();
      if (c == 0)
        usleep(100000);
      if (off == MAX_NUMBER)
        break;
    }
  }
  testVariables.streamMutex.lock();
  testVariables.testing = false;
  testVariables.streamAvail.notify_one();
  testVariables.streamMutex.unlock();
  thrPool.finish();
}
