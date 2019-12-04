include(CMakeParseArguments)

# multiValueArgs:
#   HINTS
#   INC_PATH_SUFFIX
#   LIB_PATH_SUFFIX
#   HEADERS
#   STATIC_LIBS
#   SHARED_LIBS
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

# parse arguments, rfp(rokid find package)
set(options
	REQUIRED
	RPATH
	RPATH_LINK
)
set(oneValueArgs)
set(multiValueArgs
	HINTS
	HEADERS
	STATIC_LIBS
	SHARED_LIBS
	INC_PATH_SUFFIX
	LIB_PATH_SUFFIX
)
cmake_parse_arguments(rfp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

if (rfp_REQUIRED)
set (logprio FATAL_ERROR)
else()
set (logprio STATUS)
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
	)
	if (inc_root)
		list(APPEND includes ${inc_root})
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
		NAMES lib${lib}.a
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
	list(APPEND ldflags -l${lib})
endforeach()
set(${name}_LIBRARIES ${ldflags} PARENT_SCOPE)

endfunction(findPackage)

function (git_commit_id outvar)
unset(cid CACHE)
execute_process(
	COMMAND git log
	COMMAND grep commit
	COMMAND head "-n 1"
	OUTPUT_VARIABLE cid
)
string(SUBSTRING ${cid} 7 -1 cid)
string(STRIP ${cid} cid)
set(${outvar} ${cid} PARENT_SCOPE)
endfunction(git_commit_id)
