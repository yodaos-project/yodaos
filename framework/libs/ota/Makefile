TOP=$(shell pwd)
SRC=$(wildcard  $(TOP)/bootx/*.c *.c *.cpp)

package:
	gcc ./rokid_package.c ./sha1.c -o rokid_package
unpack:
	g++ ./rokid_unpack.c ./sha1.c -I./bootx/  $(SRC) -lusb -o rokid_unpack
install:

.PHONY:  package unpack  install
clean:
	rm rokid_package
