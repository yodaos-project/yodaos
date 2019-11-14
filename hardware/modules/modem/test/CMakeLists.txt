if(MODEM_HAL_TTY)
add_executable(modem_tty_test.demo ${CMAKE_CURRENT_SOURCE_DIR}/modem_tty_test.c)
target_link_libraries(modem_tty_test.demo 
  android_hardware tinyalsa ${ANDROID_CUTILS_LIBRARIES})

install(TARGETS modem_tty_test.demo
        RUNTIME DESTINATION bin
)
else()

endif()

