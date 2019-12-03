include(CMakeParseArguments)

# multiValueArgs:
#   HINTS
#   INC_PATH_SUFFIX
#   LIB_PATH_SUFFIX
#   HEADERS
#   STATIC_LIBS
#   SHARED_LIBS
#   VERSION_SHARED_LIBS
#   VERSIONS
# options:
#   REQUIRED
#   RPATH
#   RPATH_LINK
function (findPackage name)

unset(rfp_HINTS CACHE)
unset(rfp_INC_PATH_SUFFIX CACHE)
unset(rfp_LIB_PATH_SUFFIX CACHE)
unset(rfp_HEADERS CACHE)
unset(rfp_STATIC_LIBS CACHE)
unset(rfp_SHARED_LIBS CACHE)
unset(rfp_REQUIRED CACHE)
unset(rfp_RPATH CACHE)
unset(rfp_RPATH_LINK CACHE)
unset(rfp_VERSION_SHARED_LIBS CACHE)
unset(rfp_VERSIONS CACHE)

# parse arguments, rfp(rokid find package)
set(options
  REQUIRED
  RPATH
  RPATH_LINK
)
set(multiValueArgs
  HINTS
  HEADERS
  STATIC_LIBS
  SHARED_LIBS
  INC_PATH_SUFFIX
  LIB_PATH_SUFFIX
  VERSION_SHARED_LIBS
  VERSIONS
)
cmake_parse_arguments(rfp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

if (rfp_REQUIRED)
set (logprio FATAL_ERROR)
else()
set (logprio STATUS)
endif()
unset(find_path_extra_option CACHE)
if (rfp_HINTS)
set(find_path_extra_option NO_DEFAULT_PATH)
endif()
list(APPEND rfp_HINTS /usr)
list(APPEND rfp_INC_PATH_SUFFIX include)
list(APPEND rfp_LIB_PATH_SUFFIX lib)

foreach (hfile IN LISTS rfp_HEADERS)
  unset(inc_root CACHE)
  find_path(inc_root
    NAMES ${hfile}
    HINTS ${rfp_HINTS}
    PATH_SUFFIXES ${rfp_INC_PATH_SUFFIX}
    ${find_path_extra_option}
  )
  if (inc_root)
    list(APPEND includes ${inc_root})
    message(STATUS "found ${hfile} in path ${inc_root}")
  else()
    message(${logprio} "not find ${hfile}, suffix = ${rfp_INC_PATH_SUFFIX}")
  endif()
endforeach()
if (includes)
  list (REMOVE_DUPLICATES includes)
endif()

foreach (lib IN LISTS rfp_STATIC_LIBS)
  unset(libPathName CACHE)
  find_library(
    libPathName
    NAMES ${CMAKE_STATIC_LIBRARY_PREFIX}${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}
    HINTS ${rfp_HINTS}
    PATH_SUFFIXES ${rfp_LIB_PATH_SUFFIX}
  )

  if (libPathName)
    list(APPEND archives ${libPathName})
  else()
    message(${logprio} "${name}: Not Found lib${lib}.a. HINTS ${rfp_HINTS} LIB_PATH_SUFFIX ${rfp_LIB_PATH_SUFFIX}")
  endif()
endforeach()

foreach (lib IN LISTS rfp_SHARED_LIBS)
  unset(libPathName CACHE)
  if (rfp_VERSION)
    if (${CMAKE_SHARED_LIBRARY_SUFFIX} STREQUAL ".dylib")
      set(lib ${CMAKE_SHARED_LIBRARY_PREFIX}${lib}.${rfp_VERSION}${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
      set(lib ${CMAKE_SHARED_LIBRARY_PREFIX}${lib}${CMAKE_SHARED_LIBRARY_SUFFIX}.${rfp_VERSION})
    endif()
    message(STATUS "finding library ${lib}")
  endif(rfp_VERSION)
  find_library(
    libPathName
    NAMES ${lib}
    HINTS ${rfp_HINTS}
    PATH_SUFFIXES ${rfp_LIB_PATH_SUFFIX}
  )
  if (libPathName)
    get_filename_component(libPath ${libPathName} DIRECTORY)
    list(APPEND lib_link_paths ${libPath})
    list(APPEND lib_names ${lib})
  else()
    message(${logprio} "${name}: Not Found ${lib}. HINTS ${rfp_HINTS} LIB_PATH_SUFFIX ${rfp_LIB_PATH_SUFFIX}")
  endif()
endforeach()

unset(verLibsLen CACHE)
list(LENGTH rfp_VERSION_SHARED_LIBS verLibsLen)
if (verLibsLen GREATER 0)
foreach (idx RANGE verLibsLen)
  unset(libPathName CACHE)
  list(GET rfp_VERSION_SHARED_LIBS ${idx} lib)
  list(GET rfp_VERSIONS ${idx} vernum)
  if (${CMAKE_SHARED_LIBRARY_SUFFIX} STREQUAL ".dylib")
    set(lib ${CMAKE_SHARED_LIBRARY_PREFIX}${lib}.${vernum}${CMAKE_SHARED_LIBRARY_SUFFIX})
  else()
    set(lib ${CMAKE_SHARED_LIBRARY_PREFIX}${lib}${CMAKE_SHARED_LIBRARY_SUFFIX}.${vernum})
  endif()
  find_library(
    libPathName
    NAMES ${lib}
    HINTS ${rfp_HINTS}
    PATH_SUFFIXES ${rfp_LIB_PATH_SUFFIX}
  )
  if (libPathName)
    get_filename_component(libPath ${libPathName} DIRECTORY)
    list(APPEND lib_link_paths ${libPath})
    list(APPEND ver_lib_names ${lib})
  else()
    message(${logprio} "${name}: Not Found ${lib}. HINTS ${rfp_HINTS} LIB_PATH_SUFFIX ${rfp_LIB_PATH_SUFFIX}")
  endif()
endforeach()
endif()

if (lib_link_paths)
list(REMOVE_DUPLICATES lib_link_paths)
endif()
if (lib_names)
list(REMOVE_DUPLICATES lib_names)
endif()

set(${name}_INCLUDE_DIRS ${includes} PARENT_SCOPE)
list(APPEND ldflags ${archives})
foreach (path IN LISTS lib_link_paths)
  list(APPEND ldflags -L${path})
  if (rfp_RPATH)
    list(APPEND ldflags -Wl,-rpath,${path})
  endif()
  if (rfp_RPATH_LINK)
    list(APPEND ldflags -Wl,-rpath-link,${path})
  endif()
endforeach()
foreach (lib IN LISTS lib_names)
  list(APPEND ldflags ${lib})
endforeach()
foreach (lib IN LISTS ver_lib_names)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    list(APPEND ldflags -l:${lib})
  else()
    list(APPEND ldflags ${lib})
  endif()
endforeach()
set(${name}_LIBRARIES ${ldflags} PARENT_SCOPE)

endfunction(findPackage)

function (git_commit_id outvar)
unset(cid CACHE)
if (${ARGC} GREATER 1)
  set(gitdir ${ARGV1})
else()
  set(gitdir ${CMAKE_CURRENT_SOURCE_DIR})
endif()
execute_process(
  COMMAND git show --format=%H -s
  OUTPUT_VARIABLE cid
  OUTPUT_STRIP_TRAILING_WHITESPACE
  WORKING_DIRECTORY ${gitdir}
)
set(${outvar} ${cid} PARENT_SCOPE)
endfunction(git_commit_id)