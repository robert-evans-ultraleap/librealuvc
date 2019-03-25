/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALUVC_RU_OPTION_H
#define LIBREALUVC_RU_OPTION_H

// Declare properties which can be accessed through get_pu/set_pu

namespace librealuvc {

enum ru_option {
  RU_OPTION_BACKLIGHT_COMPENSATION, /**< Enable / disable color backlight compensation*/
  RU_OPTION_BRIGHTNESS, /**< Color image brightness*/
  RU_OPTION_CONTRAST, /**< Color image contrast*/
  RU_OPTION_EXPOSURE, /**< Controls exposure time of color camera. Setting any value will disable auto exposure*/
  RU_OPTION_GAIN, /**< Color image gain*/
  RU_OPTION_GAMMA, /**< Color image gamma setting*/
  RU_OPTION_HUE, /**< Color image hue*/
  RU_OPTION_SATURATION, /**< Color image saturation setting*/
  RU_OPTION_SHARPNESS, /**< Color image sharpness setting*/
  RU_OPTION_WHITE_BALANCE, /**< Controls white balance of color image. Setting any value will disable auto white balance*/
  RU_OPTION_ENABLE_AUTO_EXPOSURE, /**< Enable / disable color image auto-exposure*/
  RU_OPTION_ENABLE_AUTO_WHITE_BALANCE, /**< Enable / disable color image auto-white-balance*/
  RU_OPTION_VISUAL_PRESET, /**< Provide access to several recommend sets of option presets for the depth camera */
  RU_OPTION_LASER_POWER, /**< Power of the F200 / SR300 projector, with 0 meaning projector off*/
  RU_OPTION_ACCURACY, /**< Set the number of patterns projected per frame. The higher the accuracy value the more patterns projected. Increasing the number of patterns help to achieve better accuracy. Note that this control is affecting the Depth FPS */
  RU_OPTION_MOTION_RANGE, /**< Motion vs. Range trade-off, with lower values allowing for better motion sensitivity and higher values allowing for better depth range*/
  RU_OPTION_FILTER_OPTION, /**< Set the filter to apply to each depth frame. Each one of the filter is optimized per the application requirements*/
  RU_OPTION_CONFIDENCE_THRESHOLD, /**< The confidence level threshold used by the Depth algorithm pipe to set whether a pixel will get a valid range or will be marked with invalid range*/
  RU_OPTION_EMITTER_ENABLED, /**< Laser Emitter enabled */
  RU_OPTION_FRAMES_QUEUE_SIZE, /**< Number of frames the user is allowed to keep per stream. Trying to hold-on to more frames will cause frame-drops.*/
  RU_OPTION_TOTAL_FRAME_DROPS, /**< Total number of detected frame drops from all streams */
  RU_OPTION_AUTO_EXPOSURE_MODE, /**< Auto-Exposure modes: Static, Anti-Flicker and Hybrid */
  RU_OPTION_POWER_LINE_FREQUENCY, /**< Power Line Frequency control for anti-flickering Off/50Hz/60Hz/Auto */
  RU_OPTION_ASIC_TEMPERATURE, /**< Current Asic Temperature */
  RU_OPTION_ERROR_POLLING_ENABLED, /**< disable error handling */
  RU_OPTION_PROJECTOR_TEMPERATURE, /**< Current Projector Temperature */
  RU_OPTION_OUTPUT_TRIGGER_ENABLED, /**< Enable / disable trigger to be outputed from the camera to any external device on every depth frame */
  RU_OPTION_MOTION_MODULE_TEMPERATURE, /**< Current Motion-Module Temperature */
  RU_OPTION_DEPTH_UNITS, /**< Number of meters represented by a single depth unit */
  RU_OPTION_ENABLE_MOTION_CORRECTION, /**< Enable/Disable automatic correction of the motion data */
  RU_OPTION_AUTO_EXPOSURE_PRIORITY, /**< Allows sensor to dynamically ajust the frame rate depending on lighting conditions */
  RU_OPTION_COLOR_SCHEME, /**< Color scheme for data visualization */
  RU_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, /**< Perform histogram equalization post-processing on the depth data */
  RU_OPTION_MIN_DISTANCE, /**< Minimal distance to the target */
  RU_OPTION_MAX_DISTANCE, /**< Maximum distance to the target */
  RU_OPTION_TEXTURE_SOURCE, /**< Texture mapping stream unique ID */
  RU_OPTION_FILTER_MAGNITUDE, /**< The 2D-filter effect. The specific interpretation is given within the context of the filter */
  RU_OPTION_FILTER_SMOOTH_ALPHA, /**< 2D-filter parameter controls the weight/radius for smoothing.*/
  RU_OPTION_FILTER_SMOOTH_DELTA, /**< 2D-filter range/validity threshold*/
  RU_OPTION_HOLES_FILL, /**< Enhance depth data post-processing with holes filling where appropriate*/
  RU_OPTION_STEREO_BASELINE, /**< The distance in mm between the first and the second imagers in stereo-based depth cameras*/
  RU_OPTION_AUTO_EXPOSURE_CONVERGE_STEP, /**< Allows dynamically ajust the converge step value of the target exposure in Auto-Exposure algorithm*/
  RU_OPTION_INTER_CAM_SYNC_MODE, /**< Impose Inter-camera HW synchronization mode. Applicable for D400/Rolling Shutter SKUs */
  RU_OPTION_STREAM_FILTER, /**< Select a stream to process */
  RU_OPTION_STREAM_FORMAT_FILTER, /**< Select a stream format to process */
  RU_OPTION_STREAM_INDEX_FILTER, /**< Select a stream index to process */
  RU_OPTION_ZOOM_ABSOLUTE,
  RU_OPTION_COUNT /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */ 
};

// Allow "RS2_" instead of "RU_", to avoid changes in backend code

typedef ru_option rs2_option;

#define RU_OPTION(name) constexpr ru_option RS2_OPTION_##name = RU_OPTION_##name;

RU_OPTION(BACKLIGHT_COMPENSATION)
RU_OPTION(BRIGHTNESS)
RU_OPTION(CONTRAST)
RU_OPTION(EXPOSURE)
RU_OPTION(GAIN)
RU_OPTION(GAMMA)
RU_OPTION(HUE)
RU_OPTION(SATURATION)
RU_OPTION(SHARPNESS)
RU_OPTION(WHITE_BALANCE)
RU_OPTION(ENABLE_AUTO_EXPOSURE)
RU_OPTION(ENABLE_AUTO_WHITE_BALANCE)
RU_OPTION(VISUAL_PRESET)
RU_OPTION(LASER_POWER)
RU_OPTION(ACCURACY)
RU_OPTION(MOTION_RANGE)
RU_OPTION(FILTER_OPTION)
RU_OPTION(CONFIDENCE_THRESHOLD)
RU_OPTION(EMITTER_ENABLED)
RU_OPTION(FRAMES_QUEUE_SIZE)
RU_OPTION(TOTAL_FRAME_DROPS)
RU_OPTION(AUTO_EXPOSURE_MODE)
RU_OPTION(POWER_LINE_FREQUENCY)
RU_OPTION(ASIC_TEMPERATURE)
RU_OPTION(ERROR_POLLING_ENABLED)
RU_OPTION(AUTO_EXPOSURE_PRIORITY)
RU_OPTION(ZOOM_ABSOLUTE)
RU_OPTION(COUNT)

} // end librealuvc

#endif
