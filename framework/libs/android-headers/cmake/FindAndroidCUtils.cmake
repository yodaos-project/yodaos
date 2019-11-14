
# - Find the android_cutils include file and library
#
#  ANDROID_CUTILS_FOUND - system has android_cutils
#  ANDROID_CUTILS_INCLUDE_DIRS - the android_cutils include directory
#  ANDROID_CUTILS_LIBRARIES - The libraries needed to use android_cutils

if(APPLE AND NOT IOS)
	set(ANDROID_CUTILS_HINTS "/usr")
endif()
if(ANDROID_CUTILS_HINTS)
	set(ANDROID_CUTILS_LIBRARIES_HINTS "${ANDROID_CUTILS_HINTS}/lib")
endif()

find_path(ANDROID_CUTILS_INCLUDE_DIRS
	NAMES cutils/ashmem.h
	HINTS "${ANDROID_CUTILS_HINTS}"
	PATH_SUFFIXES include
)

if(ANDROID_CUTILS_INCLUDE_DIRS)
	set(HAVE_ANDROID_CUTILS_H 1)
endif()

if(ENABLE_STATIC)
	find_library(ANDROID_CUTILS_LIBRARIES
		NAMES android_cutils
		HINTS "${ANDROID_CUTILS_LIBRARIES_HINTS}"
	)
else()
	find_library(ANDROID_CUTILS_LIBRARIES
		NAMES android_cutils
		HINTS "${ANDROID_CUTILS_LIBRARIES_HINTS}"
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AndroidCUtils
	DEFAULT_MSG
	ANDROID_CUTILS_INCLUDE_DIRS ANDROID_CUTILS_LIBRARIES HAVE_ANDROID_CUTILS_H
)

mark_as_advanced(ANDROID_CUTILS_INCLUDE_DIRS ANDROID_CUTILS_LIBRARIES HAVE_ANDROID_CUTILS_H)
