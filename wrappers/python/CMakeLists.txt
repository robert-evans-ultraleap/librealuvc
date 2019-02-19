#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealuvcPythonWrappers)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

set(DEPENDENCIES librealuvc)

add_subdirectory(third_party/pybind11)

set(RAW_RS_CPP 
  pybackend.cpp
  pybackend_extras.cpp
  ../../src/backend.cpp
  ../../src/linux/backend-hid.cpp
  ../../src/linux/backend-v4l2.cpp
  ../../src/log.cpp
  ../../src/types.cpp
  ../../src/win/win-backend.cpp
  ../../src/win/win-helpers.cpp
  ../../src/win/win-hid.cpp
  ../../src/win/win-usb.cpp
  ../../src/win/win-uvc.cpp
)

set(RAW_RS_HPP
  pybackend_extras.h
  ../../src/backend.h
  ../../src/linux/backend-v4l2.h
  ../../src/linux/backend-hid.h
  ../../src/types.h
  ../../include/librealuvc/realuvc.h
  ../../include/librealuvc/ru_common.h
  ../../include/librealuvc/ru_exception.h
  ../../include/librealuvc/ru_hid.h
  ../../include/librealuvc/ru_opencv.h
  ../../include/librealuvc/ru_option.h
  ../../include/librealuvc/ru_usb.h
  ../../include/librealuvc/ru_uvc.h
)

if(${BACKEND} STREQUAL "RS2_USE_LIBUVC_BACKEND")
  lappend(RAW_RS_CPP,
    ../../src/libuvc/ctrl-gen.cpp
    ../../src/libuvc/ctrl.cpp
    ../../src/libuvc/device.cpp
    ../../src/libuvc/diag.cpp
    ../../src/libuvc/frame.cpp
    ../../src/libuvc/init.cpp
    ../../src/libuvc/libuvc.cpp
    ../../src/libuvc/misc.cpp
    ../../src/libuvc/stream.cpp
  )
endif()

pybind11_add_module(pyrealuvc MODULE ${RAW_RS_CPP} ${RAW_RS_HPP})
if(USE_EXTERNAL_USB)
  #add_dependencies(pyrealuvc libusb)
endif()
target_link_libraries(pyrealuvc PRIVATE ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(pyrealuvc PROPERTIES
  VERSION   ${REALUVC_VERSION_STRING}
  SOVERSION ${REALUVC_VERSION_MAJOR}
)
set_target_properties(pyrealuvc PROPERTIES FOLDER Wrappers/python)
include_directories(pyrealuvc ../../include)

install(TARGETS pyrealuvc
  EXPORT realuvcTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
