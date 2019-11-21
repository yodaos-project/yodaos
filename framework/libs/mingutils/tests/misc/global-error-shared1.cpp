#include "test-global-error.h"

using namespace rokid;

void setGlobalError1() {
  ROKID_GERROR("shared1", 1, "error code: %d", 1);
}

void printGlobalError1() {
  printf("global error: %s\n", ROKID_GERROR_STRING);
}
