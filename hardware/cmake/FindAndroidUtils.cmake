
# - Find the android_utils include file and library
#
#  ANDROID_UTILS_FOUND - system has android_utils
#  ANDROID_UTILS_INCLUDE_DIRS - the android_utils include directory
#  ANDROID_UTILS_LIBRARIES - The libraries needed to use android_utils

if(APPLE AND NOT IOS)
	set(ANDROID_UTILS_HINTS "/usr")
endif()
if(ANDROID_UTILS_HINTS)
	set(ANDROID_UTILS_LIBRARIES_HINTS "${ANDROID_UTILS_HINTS}/lib")
endif()

find_path(ANDROID_UTILS_INCLUDE_DIRS
	NAMES utils/AndroidThreads.h
	HINTS "${ANDROID_UTILS_HINTS}"
	PATH_SUFFIXES include
)

if(ANDROID_UTILS_INCLUDE_DIRS)
	set(HAVE_ANDROID_UTILS_H 1)
endif()

if(ENABLE_STATIC)
	find_library(ANDROID_UTILS_LIBRARIES
		NAMES android_utils
		HINTS "${ANDROID_UTILS_LIBRARIES_HINTS}"
	)
else()
	find_library(ANDROID_UTILS_LIBRARIES
		NAMES android_utils
		HINTS "${ANDROID_UTILS_LIBRARIES_HINTS}"
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AndroidCUtils
	DEFAULT_MSG
	ANDROID_UTILS_INCLUDE_DIRS ANDROID_UTILS_LIBRARIES HAVE_ANDROID_UTILS_H
)

mark_as_advanced(ANDROID_UTILS_INCLUDE_DIRS ANDROID_UTILS_LIBRARIES HAVE_ANDROID_UTILS_H)
