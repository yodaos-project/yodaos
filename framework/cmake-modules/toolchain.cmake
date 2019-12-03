# this is required
SET(CMAKE_SYSTEM_NAME Linux)

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${TOOLCHAIN_HOME}/bin/${CROSS_PREFIX}gcc)
SET(CMAKE_CXX_COMPILER   ${TOOLCHAIN_HOME}/bin/${CROSS_PREFIX}g++)

# where is the target environment 
# SET(CMAKE_FIND_ROOT_PATH ${CROSS_ROOT_PATH})

# search for programs in the build host directories (not necessary)
# SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
# SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(CROSS_COMPILE_CXXFLAGS "-D__STDC_LIMIT_MACROS")

# configure Boost and Qt
# SET(QT_QMAKE_EXECUTABLE /opt/qt-embedded/qmake)
# SET(BOOST_ROOT /opt/boost_arm)