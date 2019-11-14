CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(flora-dispatcher)
set(VERSION 0.1)

option(BUILD_DEBUG "debug or release" OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
  set(COMPILER_IS_CLANG true)
else()
	set(COMPILER_IS_CLANG false)
endif()

if (BUILD_DEBUG)
	set (common_cxxflags "-g -O0")
else()
	set (common_cxxflags -O3)
	set (common_ldflags -s)
endif()
if (NOT ${COMPILER_IS_CLANG})
	set (common_ldflags "${common_ldflags} -pthread")
endif()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${common_cxxflags}")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${common_ldflags}")

include(${CUSTOM_CMAKE_MODULES}/common.mk)

git_commit_id(commit_id ${OVERRIDE_SOURCE_DIR})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIT_COMMIT_ID=${commit_id}")

findPackage(mutils REQUIRED
	HINTS ${mutilsPrefix}
	HEADERS rlog.h caps.h clargs.h
	INC_PATH_SUFFIX include/log include/caps include/misc
	SHARED_LIBS rlog caps
	STATIC_LIBS misc
)

findPackage(flora REQUIRED
	HINTS ${floraPrefix}
	HEADERS flora-svc.h
	SHARED_LIBS flora-svc
)

file(GLOB disp_src_files
	src/*.cpp
	src/*.h
)

add_executable(flora-dispatcher ${disp_src_files})
target_include_directories(flora-dispatcher PRIVATE
	src
	${mutils_INCLUDE_DIRS}
	${flora_INCLUDE_DIRS}
)
target_link_libraries(flora-dispatcher
	${mutils_LIBRARIES}
	${flora_LIBRARIES}
)
install(TARGETS flora-dispatcher
	RUNTIME DESTINATION bin
)
