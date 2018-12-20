##################################################################
# Parse librealuvc version and assign it to CMake variables    #
# This function parses librealuvc public API header file, rs.h #
# and retrieves version numbers embedded in the source code.     #
# Since the function relies on hard-coded variables, it is prone #
# for failures should these constants be modified in future      #
##################################################################
function(assign_version_property VER_COMPONENT)
    file(STRINGS "./include/librealuvc/ru.hpp" REALUVC_VERSION_${VER_COMPONENT} REGEX "#define RU_API_${VER_COMPONENT}_VERSION")
    separate_arguments(REALUVC_VERSION_${VER_COMPONENT})
    list(GET REALUVC_VERSION_${VER_COMPONENT} -1 tmp)
    if (tmp LESS 0)
        message( FATAL_ERROR "Could not obtain valid Librealuvc version ${VER_COMPONENT} component - actual value is ${tmp}" )
    endif()
    set(REALUVC_VERSION_${VER_COMPONENT} ${tmp} PARENT_SCOPE)
endfunction()

set(REALUVC_VERSION_MAJOR -1)
set(REALUVC_VERSION_MINOR -1)
set(REALUVC_VERSION_PATCH -1)
assign_version_property(MAJOR)
assign_version_property(MINOR)
assign_version_property(PATCH)
set(REALUVC_VERSION_STRING ${REALUVC_VERSION_MAJOR}.${REALUVC_VERSION_MINOR}.${REALUVC_VERSION_PATCH})
infoValue(REALUVC_VERSION_STRING)
