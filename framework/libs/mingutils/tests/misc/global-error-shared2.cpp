#include "test-global-error.h"

using namespace rokid;

void setGlobalError2() {
  ROKID_GERROR("shared2", 2, "error code: %d", 2);
}

void printGlobalError2() {
  printf("global error: %s\n", ROKID_GERROR_STRING);
}
