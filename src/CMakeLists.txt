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

if(LRS_TRY_USE_AVX)
    set_source_files_properties(image_avx.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

if(BUILD_SHARED_LIBS)
    target_sources(${LRS_TARGET} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/realsense.def")
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../include)

if(NOTDEF)
target_sources(${LRS_TARGET}
    PRIVATE
        # "${CMAKE_CURRENT_LIST_DIR}/environment.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/device_hub.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/archive.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/context.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sensor.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/algo.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/sync.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/stream.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/option.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/error-handling.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/hw-monitor.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/image.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/image_avx.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/log.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/rs.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/types.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/backend.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/verify.c"
        # "${CMAKE_CURRENT_LIST_DIR}/software-device.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/source.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/software-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/environment.h"
        "${CMAKE_CURRENT_LIST_DIR}/device_hub.h"
        "${CMAKE_CURRENT_LIST_DIR}/archive.h"
        "${CMAKE_CURRENT_LIST_DIR}/concurrency.h"
        "${CMAKE_CURRENT_LIST_DIR}/context.h"
        "${CMAKE_CURRENT_LIST_DIR}/sensor.h"
        "${CMAKE_CURRENT_LIST_DIR}/sync.h"
        "${CMAKE_CURRENT_LIST_DIR}/sensor.h"
        "${CMAKE_CURRENT_LIST_DIR}/stream.h"
        "${CMAKE_CURRENT_LIST_DIR}/algo.h"
        "${CMAKE_CURRENT_LIST_DIR}/option.h"
        "${CMAKE_CURRENT_LIST_DIR}/metadata.h"
        "${CMAKE_CURRENT_LIST_DIR}/metadata-parser.h"
        "${CMAKE_CURRENT_LIST_DIR}/error-handling.h"
        "${CMAKE_CURRENT_LIST_DIR}/hw-monitor.h"
        "${CMAKE_CURRENT_LIST_DIR}/image.h"
        "${CMAKE_CURRENT_LIST_DIR}/image_avx.h"
        "${CMAKE_CURRENT_LIST_DIR}/source.h"
        "${CMAKE_CURRENT_LIST_DIR}/types.h"
        "${CMAKE_CURRENT_LIST_DIR}/backend.h"
        "${CMAKE_CURRENT_LIST_DIR}/device.h"
        "${CMAKE_CURRENT_LIST_DIR}/api.h"
)
endif()

add_executable(
    mytest
	    "${CMAKE_CURRENT_LIST_DIR}/mytest.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/environment.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/device_hub.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/archive.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/context.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/device.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/sensor.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/algo.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/sync.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/stream.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/option.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/error-handling.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/hw-monitor.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/image.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/image_avx.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/log.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/rs.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/types.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/backend.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/verify.c"
        # "${CMAKE_CURRENT_LIST_DIR}/software-device.cpp"
        # "${CMAKE_CURRENT_LIST_DIR}/source.cpp"
		
        "${CMAKE_CURRENT_LIST_DIR}/win/win-helpers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-usb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win/win-backend.cpp"
		
)

