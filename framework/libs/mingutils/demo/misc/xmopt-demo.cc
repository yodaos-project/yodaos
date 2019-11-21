#include "xmopt.h"

using namespace std;

void printOption(XMOptions& options, const char* key) {
  auto r = options.find(key);
  while (true) {
    auto opt = r.next();
    if (opt == nullptr)
      break;
    printf("key %s, value %s, integer %d\n", opt->key(), opt->value(), opt->integer());
  }
}

int main(int argc, char **argv) {
  XMOptions options;
	options.option("h", "help", "print this message");
  options.option(nullptr, "foo", "example param 1");
  options.option("b", "bar", "example param 2");
  if (!options.parse(argc, argv)) {
    string msg;
    options.errorMsg(msg);
    printf("%s\n", msg.c_str());
		options.prompt(msg);
		printf("%s\n", msg.c_str());
    return 1;
  }

  printOption(options, "foo");
  printOption(options, "b");
  printOption(options, nullptr);
  return 0;
}
