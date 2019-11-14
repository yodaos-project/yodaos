
if(MIC_ARRAY_HAL_VSP)
add_executable(mic_array_test.demo ${CMAKE_CURRENT_SOURCE_DIR}/mic_array_vsp_test.c)
elseif(MIC_ARRAY_HAL_PCM)
add_executable(mic_array_test.demo ${CMAKE_CURRENT_SOURCE_DIR}/mic_array_pcm_test.c)
else()
add_executable(mic_array_test.demo ${CMAKE_CURRENT_SOURCE_DIR}/mic_array_test.c)
endif()
target_link_libraries(mic_array_test.demo 
  android_hardware tinyalsa ${ANDROID_CUTILS_LIBRARIES})

install(TARGETS mic_array_test.demo
        RUNTIME DESTINATION bin
)
