set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I./ -I../include -I./include  -Wall -O2 -g -ggdb -Wno-unused-function -Wno-unused-variable -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I./ -I../include -I./include  -Wall -O2 -g -ggdb -Wno-unused-function -Wno-unused-variable")

if (ROKIDOS_FEATURES_HAS_GUI)
    add_definitions(-DROKIDOS_FEATURES_HAS_GUI=$(ROKIDOS_FEATURES_HAS_GUI))
    message(STATUS "Use GUI")
endif()

#if (FFMPEG_HTTP_LOW_CACHE)
    add_definitions(-DFFMPEG_HTTP_LOW_CACHE)
    message(STATUS "Use ffmpeg http low cache")
#endif()

#if (BLOCK_PAUSE_MODE)
    add_definitions(-DBLOCK_PAUSE_MODE)
    message(STATUS "Use Block pause mode")
#endif()

INCLUDE_DIRECTORIES(./)
if (ROKID_LIBS_SOUNDTOUCH)
    set(SRC_FILES_RPLAYER
	./MediaPlayer.cpp
	./rk_ffplay.c
	./RKSoundTouch.cpp
     )
else()
    set(SRC_FILES_RPLAYER
        ./MediaPlayer.cpp
        ./rk_ffplay.c
     )
endif()

add_library(rplayer SHARED ${SRC_FILES_RPLAYER})
target_link_libraries(rplayer avformat avcodec swscale avutil swresample avdevice pulse pulse-simple pthread SDL2 rklog)

if (ROKID_LIBS_SOUNDTOUCH)
    add_definitions(-DROKIDOS_FEATURES_HAS_SOUNDTOUCH)
    target_link_libraries(rplayer SoundTouch)
endif()

if(ROKID_LIBS_EQ_CTRL)
    add_definitions(-DROKIDOS_FEATURES_HAS_EQ_CTRL)
    target_link_libraries(rplayer rkeqcontrol)
endif()

target_link_libraries(rplayer ${DEPENDENCIES})

install(TARGETS rplayer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

set(SRC_FILES_WAVPLAYER
    ./WavPlayer.cpp
    ./SimplePlayer.cpp
)
add_library(wavplayer SHARED ${SRC_FILES_WAVPLAYER})
if (NEED_AUDIO_PRE_LINK)
   add_definitions(-DNEED_WAVPLAY_PRE_LINK)
endif()
target_link_libraries(wavplayer pulse pulse-simple pthread rklog sndfile)
install(TARGETS wavplayer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

set(SRC_FILES_OPUSPLAYER
    ./OpusCodec.cpp
    ./OpusPlayer.cpp
)
add_library(opusplayer SHARED ${SRC_FILES_OPUSPLAYER})
target_link_libraries(opusplayer opus pulse-simple pthread rklog)
install(TARGETS opusplayer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
