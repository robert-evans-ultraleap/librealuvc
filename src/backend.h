// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALUVC_BACKEND_H
#define LIBREALUVC_BACKEND_H

#include "../include/librealuvc/ru.hpp"     // Inherit all type definitions in the public API
#include <algorithm>
#include <chrono>
#include <memory>       // For shared_ptr

const uint16_t MAX_RETRIES                = 100;
const uint16_t VID_INTEL_CAMERA           = 0x8086;
const uint8_t  DEFAULT_V4L2_FRAME_BUFFERS = 4;
const uint16_t DELAY_FOR_RETRIES          = 50;

const uint8_t MAX_META_DATA_SIZE          = 0xff; // UVC Metadata total length
                                            // is limited by (UVC Bulk) design to 255 bytes
namespace librealuvc {

// In the original librealsense code, all the backend declarations
// were in librealuvc::platform.  To avoid unnecessary changes to the
// backend, we now declare the outer types, and alias them in platform.

namespace platform {

typedef librealuvc::backend backend;
typedef librealuvc::backend_device_group backend_device_group;
typedef librealuvc::control_range control_range;
typedef librealuvc::frame_object frame_object;
typedef librealuvc::notification notification;
typedef librealuvc::power_state power_state;
typedef librealuvc::time_service time_service;
typedef librealuvc::guid guid;
typedef librealuvc::extension_unit extension_unit;
typedef librealuvc::stream_profile_tuple stream_profile_tuple;
typedef librealuvc::stream_profile stream_profile;
        
std::shared_ptr<backend> create_backend();

} // end platform

double monotonic_to_realtime(double monotonic);

} // end librealuvc

#endif
