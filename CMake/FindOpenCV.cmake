if(BUILD_ANDROID AND (NOT OpenCV_DIR OR OpenCV_DIR MATCHES "NOTFOUND"))
  if(BUILD_ANDROID64)
    set(_abi_name "arm64-v8a")
  else()
    set(_abi_name "armeabi-v7a")
  endif()
  set(OpenCV_DIR "${EXTERNAL_LIBRARY_DIR}/opencv-3.2/sdk/native/jni/abi-${_abi_name}" CACHE PATH "path to JNI OpenCVConfig.cmake" FORCE)
  set(OpenCV_PREFERRED 3.2)
else()
  set(OpenCV_DIR "${EXTERNAL_LIBRARY_DIR}/opencv-4.0" CACHE PATH "path to OpenCV library" FORCE)
  set(OpenCV_PREFERRED 4.0)
endif()
find_package(OpenCV ${OpenCV_PREFERRED} REQUIRED NO_MODULE)
find_package_handle_standard_args(OpenCV
  DEFAULT_MSG
  OpenCV_INCLUDE_DIRS OpenCV_LIBS OpenCV_SHARED
)
