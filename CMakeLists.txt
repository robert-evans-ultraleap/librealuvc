#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)
project(librealuvc LANGUAGES CXX C)

# include librealsense general configuration
include(CMake/global_config.cmake)

config_cxx_flags()

# include os specific macros
# macro definition located at "CMake/global_config.cmake"
include(CMake/include_os.cmake)

# set os specific configuration flags
# macro definition located at "CMake/<OS>_config.cmake"
os_set_flags()

# set global configuration flags
# macro definition located at "CMake/global_config.cmake"
global_set_flags()

if(BUILD_SHARED_LIBS)
    add_library(${LRS_TARGET} SHARED "")
else()
    add_library(${LRS_TARGET} STATIC "")
endif()

# set library version
set_target_properties(${LRS_TARGET} PROPERTIES VERSION ${REALUVC_VERSION_STRING} SOVERSION ${REALUVC_VERSION_MAJOR})

# add_OpenCV()

include(include/CMakeLists.txt)
include(src/CMakeLists.txt)
include(third-party/CMakeLists.txt)

# configure the project according to OS specific requirments
# macro definition located at "CMake/<OS>_config.cmake"
os_target_config()

# global project configuration
# macro definition located at "CMake/global_config.cmake"
global_target_config()

add_subdirectory(wrappers)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_UNIT_TESTS)
    add_subdirectory(unit-tests)
endif()

include(CMake/install_config.cmake)