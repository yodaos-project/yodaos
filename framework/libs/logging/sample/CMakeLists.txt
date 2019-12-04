include_directories(${CMAKE_SOURCE_DIR})

add_executable(logSample logSample.cpp)
target_link_libraries(logSample rklog)

install(TARGETS logSample
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

add_executable(logSampleC logSampleC.c)
target_link_libraries(logSampleC rklog)
install(TARGETS logSampleC
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)