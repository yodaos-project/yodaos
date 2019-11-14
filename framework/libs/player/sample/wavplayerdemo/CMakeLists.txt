add_executable(wavplayer_demo WavDemo.cpp)
target_link_libraries(wavplayer_demo wavplayer)
target_link_libraries(wavplayer_demo -Wl,-rpath-link,${CMAKE_PREFIX_PATH}/usr/lib/pulseaudio)

install(TARGETS wavplayer_demo
    RUNTIME DESTINATION bin
)
