#include <stdio.h>
#include "caps.h"

using namespace std;

uint32_t caps_size0() {
	shared_ptr<Caps> caps = Caps::new_instance();
	return caps->binary_size();
}

uint32_t caps_size1() {
	shared_ptr<Caps> caps = Caps::new_instance();
	caps->write(0);
	caps->write((int64_t)1000);
	caps->write(1);
	return caps->binary_size();
}

uint32_t caps_size2() {
	shared_ptr<Caps> caps = Caps::new_instance();
	shared_ptr<Caps> sub = Caps::new_instance();
	caps->write("hello world");
	caps->write(2);
	caps->write("test caps size, too large");
	sub->write("i am a sub caps");
	sub->write("it's work very well");
	sub->write(nullptr, 0);
	caps->write(sub);
	return caps->binary_size();
}

int main(int argc, char** argv) {
	printf("caps 0 size = %u\n", caps_size0());
	printf("caps 1 size = %u\n", caps_size1());
	printf("caps 2 size = %u\n", caps_size2());
	return 0;
}
