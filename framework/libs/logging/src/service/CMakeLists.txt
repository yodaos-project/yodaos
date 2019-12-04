add_executable(cloudlog cloudlog.c)
target_link_libraries(cloudlog dl pthread log_c_sdklite curl android_cutils)
install(TARGETS cloudlog
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)
find_library(json NAMES json-c json)
ADD_EXECUTABLE(log2cloud log2cloud.c)
TARGET_LINK_LIBRARIES(log2cloud pthread ubox ubus ${json} blobmsg_json log_c_sdklite curl android_cutils cjson rklog)

target_include_directories(log2cloud PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(log2cloud
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)
install(TARGETS log2cloud
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)