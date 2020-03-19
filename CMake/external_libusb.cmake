message(STATUS "Use external libusb")

include(FetchContent)
FetchContent_Declare(libusb
    GIT_REPOSITORY "https://github.com/libusb/libusb.git"
    GIT_TAG "v1.0.23"

    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third-party/libusb/CMakeLists.txt
            <SOURCE_DIR>
)

FetchContent_GetProperties(libusb)
if(NOT libusb_POPULATED)
  FetchContent_Populate(libusb)
  add_subdirectory(${libusb_SOURCE_DIR} ${libusb_BINARY_DIR})
endif()

set(LIBUSB1_LIBRARY_DIRS ${libusb_BINARY_DIR})
link_directories(${LIBUSB1_LIBRARY_DIRS})

set(LIBUSB1_LIBRARIES usb)
#set(LIBUSB_LOCAL_INCLUDE_PATH third-party/libusb)

set(USE_EXTERNAL_USB ON)
set(LIBUSB_LOCAL_INCLUDE_PATH ${libusb_BINARY_DIR})
