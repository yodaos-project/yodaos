add_library(input-event SHARED input-event.c)
target_link_libraries(input-event rklog)
install( TARGETS input-event ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_library(input-event-hot SHARED input-event-hot.c)
target_link_libraries(input-event-hot rklog)
install( TARGETS input-event-hot ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
