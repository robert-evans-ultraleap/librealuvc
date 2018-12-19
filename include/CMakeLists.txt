target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/rs.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/rs.h"

        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_types.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_context.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_device.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_frame.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_option.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_processing.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_record_playback.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_sensor.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_internal.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_pipeline.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_config.h"

        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_types.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_context.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_device.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_frame.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_processing.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_record_playback.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_sensor.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_internal.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/hpp/rs_pipeline.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/rsutil.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/rs_advanced_mode.h"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/rs_advanced_mode.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/librealuvc/h/rs_advanced_mode_command.h"
)
