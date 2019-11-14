
if (BR2_PACKAGE_ANDROID_HARDWARE_A113_PCM_DEVICEID STREQUAL "y")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DA113_PCM_DEVICEID")
endif()

if(MIC_ARRAY_HAL_VSP)
add_library(mic_array.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/mic_array_vsp.c)
elseif(MIC_ARRAY_HAL_PCM)
add_library(mic_array.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/mic_array_pcm.c)
else()
add_library(mic_array.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/mic_array.c)
endif()

target_link_libraries(mic_array.default 
  android_hardware tinyalsa ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS mic_array.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(test)

