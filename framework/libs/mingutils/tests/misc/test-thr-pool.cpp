#include <atomic>
#include "gtest/gtest.h"
#include "thr-pool.h"

using namespace std;

TEST(ThreadPool, doTask) {
  ThreadPool pool;
  uint32_t taskDone{0};
  uint32_t taskNum{100};
  pool.init(1);
  uint32_t i;
  for (i = 0; i < taskNum; ++i) {
    pool.push([&taskDone]() {
      ++taskDone;
    });
  }
  pool.finish();
  EXPECT_EQ(taskDone, taskNum);
}

TEST(ThreadPool, queueTask) {
  ThreadPool pool;
  uint32_t maxThreads = 5;
  uint32_t taskNum = 10;
  uint32_t i;
  uint32_t repeat{0};
  const static uint32_t REPEAT_NUM = 3;
  struct {
    atomic<uint32_t> running{0};
    atomic<uint32_t> maxRunning{0};
    atomic<uint32_t> done{0};
  } vars;

  pool.init(maxThreads);
  while (repeat < REPEAT_NUM) {
    ++repeat;
    for (i = 0; i < taskNum; ++i) {
      pool.push([&vars]() {
        ++vars.running;
        if (vars.maxRunning < vars.running)
          vars.maxRunning = vars.running.load();
        sleep(5);
        --vars.running;
        ++vars.done;
      });
    }
    pool.finish();
    EXPECT_EQ(vars.maxRunning.load(), maxThreads);
    EXPECT_EQ(vars.done.load(), taskNum);
    EXPECT_EQ(vars.running.load(), 0);
    vars.running = 0;
    vars.maxRunning = 0;
    vars.done = 0;
  }
}
