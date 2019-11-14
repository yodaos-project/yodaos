
add_executable(bt_demo ${CMAKE_CURRENT_SOURCE_DIR}/bluetooth_test.c)
target_link_libraries(bt_demo 
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

INSTALL( TARGETS bt_demo ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})


add_executable(input_demo ${CMAKE_CURRENT_SOURCE_DIR}/input.c)
