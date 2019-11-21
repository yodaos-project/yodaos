#include <string.h>
#include <stdio.h>
#include <memory>
#include "clargs.h"

using namespace std;

typedef struct {
	int argc;
	char** argv;
} CmdLineArgs;
static CmdLineArgs test_args[] = {
	5, (char*[]){ "foo", "-", "--", "---", "=", "."},
	3, (char*[]){ "foo", "-x", "-i", "-?"},
	5, (char*[]){ "foo", "-abc", "--help", "--test=", "--hello=world"},
	4, (char*[]){ "foo", "-x", "efg", "-x"},
	3, (char*[]){ "foo", "--test=--hi", "--test=haha"},
	3, (char*[]){ "foo", "--test", "--test=haha"},
	7, (char*[]){ "foo", "---test", "--test=haha", "--err1==haha", "--err2=.haha", "--err3=123=456", "--ok=100"},
	3, (char*[]){ "foo", "-x1", "-i.0", "-o", "---" },
	10, (char*[]){ "foo", "-i", "speech_inc", "-x", "--include=/usr/include", "--foo-bar", "-rvb", "foo.c", "bar.c", "helloworld" }
};

const char* visible_string(int c, char** v) {
	static char buf[256];
	char* p = buf;
	int i;
	size_t l;
	for (i = 0; i < c; ++i) {
		if (i) {
			*p = ' ';
			++p;
		}
		l = strlen(v[i]);
		memcpy(p, v[i], l);
		p += l;
	}
	*p = '\0';
	return buf;
}

void display_parse_result(shared_ptr<CLArgs>& handle) {
	const char* key;
	const char* value;
  uint32_t i;
  uint32_t size = handle->size();

  CLPair pair;
	printf("[pairs key value]\n");
  for (i = 0; i < size; ++i) {
		printf("\t%s: %s\n", handle->at(i, pair).key, handle->at(i, pair).value);
	}
}

int main(int argc, char** argv) {
	size_t count = sizeof(test_args) / sizeof(CmdLineArgs);
	size_t i;
  shared_ptr<CLArgs> handle;

	for (i = 0; i < count; ++i) {
		handle = CLArgs::parse(test_args[i].argc, test_args[i].argv);
		if (handle == nullptr) {
			printf("parse %s failed.\n", visible_string(test_args[i].argc, test_args[i].argv));
			break;
		}
		printf("--parse command line: \"%s\"--\n", visible_string(test_args[i].argc, test_args[i].argv));
		display_parse_result(handle);
	}
	return 0;
}
