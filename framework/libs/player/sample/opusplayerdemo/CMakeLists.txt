add_executable(opusplayer_demo OpusDemo.cpp)
target_link_libraries(opusplayer_demo opusplayer)
target_link_libraries(opusplayer_demo -Wl,-rpath-link,${CMAKE_PREFIX_PATH}/usr/lib/pulseaudio)

install(TARGETS opusplayer_demo
    RUNTIME DESTINATION bin
)
