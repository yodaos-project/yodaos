if(ETHNET_HAL)
add_library(ethnet_hal.default SHARED ${CMAKE_CURRENT_SOURCE_DIR}/ethnet_hw.c)

add_subdirectory(test)

target_link_libraries(ethnet_hal.default
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS ethnet_hal.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

endif()
