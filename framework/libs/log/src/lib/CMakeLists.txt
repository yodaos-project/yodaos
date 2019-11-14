if(DEFINED BOARDCONFIG_LOG_LEVEL)
    ADD_DEFINITIONS(-DROKID_LOG_LEVEL=${BOARDCONFIG_LOG_LEVEL})
endif()
add_library(rklog SHARED RKLog.cpp RKLogServer.cpp)
#target_link_libraries(rklog dl pthread)
target_include_directories(rklog PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(rklog
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)
install(TARGETS rklog
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)