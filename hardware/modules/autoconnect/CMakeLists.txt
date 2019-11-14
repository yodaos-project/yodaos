if(WIFI_HAL)
add_library(autoconnect.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/wifi_autoconncet_hw.c)

target_link_libraries(autoconnect.default
  android_hardware wpa_ctrl ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS autoconnect.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(autoconnect_hw_test)
endif()
