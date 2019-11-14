set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -I../include -std=c++11" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -I../include" )

INCLUDE(GNUInstallDirs)

set(PAMIXER_SOURCES 
	./callbacks.cc
	./device.cc
	./pulseaudio.cc
	./callbacks.hh 
	./device.hh
	./pulseaudio.hh
	./vol_ctrl.cc
)
add_library(rkvolumecontrol SHARED ${PAMIXER_SOURCES})
target_link_libraries(rkvolumecontrol pulse-simple pulse pulse-mainloop-glib)

install(TARGETS rkvolumecontrol ${INSTALL_EXPORT}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

file(GLOB INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/volumecontrol.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vol_ctrl)
