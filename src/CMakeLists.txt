string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})
include(${_rel_path}/core/CMakeLists.txt)
include(${_rel_path}/linux/CMakeLists.txt)
include(${_rel_path}/res/CMakeLists.txt)

if(FORCE_LIBUVC)
    include(${_rel_path}/libuvc/CMakeLists.txt)
endif()

if(BUILD_FOR_OTHER_WIN)
    include(${_rel_path}/win/CMakeLists.txt)
endif()

if(BUILD_SHARED_LIBS)
    add_compile_options("-DLIBREALUVC_BUILD_SHARED=1 -DLIBREALUVC_EXPORTS=1")
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../include)

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/backend.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/log.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/types.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/videocapture.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/api.h"
        "${CMAKE_CURRENT_LIST_DIR}/backend.h"
        "${CMAKE_CURRENT_LIST_DIR}/concurrency.h"
        "${CMAKE_CURRENT_LIST_DIR}/leap_xu.h"
        "${CMAKE_CURRENT_LIST_DIR}/types.h"
		
        "${CMAKE_CURRENT_LIST_DIR}/win/win-helpers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-usb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-backend.cpp"
		
)

#add_executable(
#    mytest
#	    "${CMAKE_CURRENT_LIST_DIR}/mytest.cpp"
#)

