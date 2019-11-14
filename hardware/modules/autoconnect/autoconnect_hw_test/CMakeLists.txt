
add_executable(autoconnect_hw_demo ${CMAKE_CURRENT_SOURCE_DIR}/autoconnect_hw_test.c)
target_link_libraries(autoconnect_hw_demo 
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS autoconnect_hw_demo ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
