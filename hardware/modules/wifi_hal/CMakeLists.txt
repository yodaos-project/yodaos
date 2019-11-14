if(WIFI_HAL)
add_library(wifi_hal.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/wifi_hw.c)

add_subdirectory(test)

target_link_libraries(wifi_hal.default
  android_hardware rklog ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS wifi_hal.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

endif()
