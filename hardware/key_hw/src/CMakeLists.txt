set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -I./ -I../include -Iinclude -DRK_FLORA -Wall -g -ggdb -Wno-unused-function" )

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(SRC_FILES
	./key_hw.c
)

add_library(key_hw SHARED ${SRC_FILES} ${COMMON_FILES} ${CLIENT_FILES})

install(TARGETS key_hw
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)


