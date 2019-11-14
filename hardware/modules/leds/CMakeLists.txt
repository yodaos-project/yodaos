if(LED_HAL_PWM)
add_library(led_array.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/leds_pwm_hw.c)
elseif(LED_HAL_I2C)
add_library(led_array.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/leds_i2c_hw.c)
elseif(LED_HAL_YODASTAGE)
add_library(led_array.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/leds_yodastage_hw.c)
else()
add_library(led_array.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/leds_hw.c)
endif()

add_subdirectory(test)

target_link_libraries(led_array.default
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS led_array.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

