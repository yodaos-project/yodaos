
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..")

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBPULSE-SIMPLE REQUIRED libpulse-simple)
link_directories(${LIBPULSE-SIMPLE_STATIC_LIBRARY_DIRS})

add_executable(vctest vctest.c)
target_link_libraries(vctest rkvolumecontrol)
target_link_libraries(vctest ${LIBPULSE-SIMPLE_STATIC_LIBRARIES})
