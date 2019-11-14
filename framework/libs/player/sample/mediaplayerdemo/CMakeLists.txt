add_executable(rplayer_demo MediaPlayerDemo.cpp)
target_link_libraries(rplayer_demo rplayer)
target_link_libraries(rplayer_demo avformat)
target_link_libraries(rplayer_demo avutil)
target_link_libraries(rplayer_demo -Wl,-rpath-link,${CMAKE_PREFIX_PATH}/usr/lib/pulseaudio)

install(TARGETS rplayer_demo
    RUNTIME DESTINATION bin
)
