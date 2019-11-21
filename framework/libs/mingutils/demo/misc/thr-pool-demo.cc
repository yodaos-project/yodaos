#include <unistd.h>
#include <atomic>
#include "thr-pool.h"

using namespace std;

int main(int argc, char **argv) {
  ThreadPool pool(10);
  atomic<uint32_t> counter;
  auto taskfunc = [&counter]() {
    usleep(100000);
    ++counter;
  };
  uint32_t i;
  uint32_t total = 123;
  for (i = 0; i < total; ++i) {
    pool.push(taskfunc);
  }
  pool.finish();

  printf("counter == %d, excepted %d\n", counter.load(), total);
  return 0;
}
