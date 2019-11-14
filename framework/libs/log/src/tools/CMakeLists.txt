add_executable(set_log_level set_log_level.c)
install(TARGETS set_log_level
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)
add_executable(rklogger rklogger.c)
install(TARGETS rklogger
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)