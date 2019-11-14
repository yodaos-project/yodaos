if(MODEM_HAL_TTY)
add_library(modem_tty.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/modem_tty_hw.c)
add_subdirectory(test)

target_link_libraries(modem_tty.default
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

target_link_libraries(modem_tty.default json-c)
target_link_libraries(modem_tty.default pthread)
target_link_libraries(modem_tty.default usb-1.0)
target_link_libraries(modem_tty.default rklog)

INSTALL( TARGETS modem_tty.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

else()

endif()

