#include "gtest/gtest.h"
#include "test-global-error.h"

using namespace rokid;

TEST(GlobalError, simple) {
  setGlobalError1();
  printGlobalError1();
  printGlobalError2();
  setGlobalError2();
  printGlobalError1();
  printGlobalError2();
}
