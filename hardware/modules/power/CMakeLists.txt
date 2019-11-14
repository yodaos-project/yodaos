if(POWER_HAL)
add_library(power_hal.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/power_hw.c)

add_subdirectory(test)

target_link_libraries(power_hal.default
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS power_hal.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

endif()
