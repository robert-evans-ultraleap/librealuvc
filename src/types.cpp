// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "types.h"
#include <librealuvc/realuvc.h>
#include <librealuvc/ru_uvc.h>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <thread>
#include <vector>

namespace librealuvc {

using std::shared_ptr;
using std::string;
using std::vector;

backend_device_group::backend_device_group() { }

backend_device_group::backend_device_group(
  const vector<uvc_device_info>& uvc_devs,
  const vector<usb_device_info>& usb_devs,
  const vector<hid_device_info>& hid_devs
) :
  uvc_devices(uvc_devs),
  usb_devices(usb_devs),
  hid_devices(hid_devs) {
}

OpaqueCalibration::OpaqueCalibration(
  const string& format_name,
  int major,
  int minor,
  int patch,
  const vector<uint8_t>& data
) :
  format_name_(format_name),
  version_major_(major),
  version_minor_(minor),
  version_patch_(patch),
  data_(data) {
}

// stream_profile is declared in <librealuvc/hpp/ru_common.hpp>

bool stream_profile::operator==(const stream_profile& b) const {
  return ((width == b.width) && (height == b.height) && 
    (fps == b.fps) && (format == b.format));
}

stream_profile::operator stream_profile_tuple() const {
  return std::make_tuple(width, height, fps, format);
}

ru_time_t os_time_service::get_time() const {
  using namespace std::chrono;
  return duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
}

const std::map<usb_spec, string> usb_spec_names = {
  { usb_undefined, "Undefined" },
  { usb1_type,     "1.0" },
  { usb1_1_type,   "1.1" },
  { usb2_type,     "2.0" },
  { usb2_1_type,   "2.1" },
  { usb3_type,     "3.0" },
  { usb3_1_type,   "3.1" },
  { usb3_2_type,   "3.2" }
};

uvc_device_info::uvc_device_info() :
  id(""),
  vid(0),
  pid(0),
  mi(0),
  unique_id(""),
  device_path(""),
  conn_spec(usb_undefined),
  uvc_capabilities(0),
  has_metadata_node(false),
  metadata_node_id("") {
}

bool uvc_device_info::operator==(const uvc_device_info& b) const {
  return (
    (vid == b.vid) &&
    (pid == b.pid) &&
    (mi == b.mi) &&
    (unique_id == b.unique_id) &&
    (id == b.id) &&
    (device_path == b.device_path) &&
    (conn_spec == b.conn_spec)
  );
}

// A uvc_device wrapper which retires get/set_pu and get/set_xu calls

static constexpr int MAX_RETRIES = 40;
static constexpr int DELAY_FOR_RETRIES = 100;

uvc_device_with_retry::uvc_device_with_retry(shared_ptr<uvc_device> raw) :
  raw_(raw) {
}

void uvc_device_with_retry::probe_and_commit(stream_profile prof, frame_callback callback, int buffers) {
  raw_->probe_and_commit(prof, callback, buffers);
}

void uvc_device_with_retry::stream_on(error_callback on_error) {
  raw_->stream_on(on_error);
}

void uvc_device_with_retry::start_callbacks() {
  raw_->start_callbacks();
}

void uvc_device_with_retry::stop_callbacks() {
  raw_->stop_callbacks();
}

void uvc_device_with_retry::close(stream_profile prof) {
  raw_->close(prof);
}

void uvc_device_with_retry::set_power_state(power_state state) {
  raw_->set_power_state(state);
}

power_state uvc_device_with_retry::get_power_state() const {
  return raw_->get_power_state();
}

void uvc_device_with_retry::init_xu(const extension_unit& xu) {
  raw_->init_xu(xu);
}

bool uvc_device_with_retry::set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) {
  auto snooze = std::chrono::milliseconds(DELAY_FOR_RETRIES);
  for (auto j = 0;;) {
    if (raw_->set_xu(xu, ctrl, data, len)) return true;
    if (++j >= MAX_RETRIES) break;
    std::this_thread::sleep_for(snooze);
  }
  return false;
}

bool uvc_device_with_retry::get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const {
  auto snooze = std::chrono::milliseconds(DELAY_FOR_RETRIES);
  for (auto j = 0;;) {
    if (raw_->get_xu(xu, ctrl, data, len)) return true;
    if (++j >= MAX_RETRIES) break;
    std::this_thread::sleep_for(snooze);
  }
  return false;
}

control_range uvc_device_with_retry::get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const {
  return raw_->get_xu_range(xu, ctrl, len);
}

bool uvc_device_with_retry::get_pu(ru_option opt, int32_t& value) const {
  auto snooze = std::chrono::milliseconds(DELAY_FOR_RETRIES);
  for (auto j = 0;;) {
    if (raw_->get_pu(opt, value)) return true;
    if (++j >= MAX_RETRIES) break;
    std::this_thread::sleep_for(snooze);
  }
  return false;
}

bool uvc_device_with_retry::set_pu(ru_option opt, int32_t value) {
  auto snooze = std::chrono::milliseconds(DELAY_FOR_RETRIES);
  for (auto j = 0;;) {
    if (raw_->set_pu(opt, value)) return true;
    if (++j >= MAX_RETRIES) break;
    std::this_thread::sleep_for(snooze);
  }
  return false;
}

control_range uvc_device_with_retry::get_pu_range(ru_option opt) const {
  return raw_->get_pu_range(opt);
}

vector<stream_profile> uvc_device_with_retry::get_profiles() const {
  return raw_->get_profiles();
}

void uvc_device_with_retry::lock() const {
  raw_->lock();
}

void uvc_device_with_retry::unlock() const {
  raw_->unlock();
}

string uvc_device_with_retry::get_device_location() const {
  return raw_->get_device_location();
}

usb_spec uvc_device_with_retry::get_usb_specification() const {
  return raw_->get_usb_specification();
}

// Converting various structs to strings

#define MEMBER(name) { \
  string val = fmt(name); \
  ss << "  " << #name << ": " << val << "," << "\n"; \
}
template<typename T>
inline string fmt(T val) {
  std::stringstream ss; ss << val; return ss.str();
};

template<>
inline string fmt(uint8_t val) { char buf[64]; sprintf(buf, "0x%02x", (int)val); return string(buf); };
template<>
inline string fmt(uint16_t val) { char buf[64]; sprintf(buf, "0x%04x", (int)val); return string(buf); };
template<>
inline string fmt(uint32_t val) { char buf[64]; sprintf(buf, "0x%08x", (int)val); return string(buf); };
template<>
inline string fmt(const string& val) {
  std::stringstream ss;
  ss << '\"' << val << '\"';
  return ss.str();
};

string stream_profile::to_string() const {
  std::stringstream ss;
  ss << "{\n";
  MEMBER(width);
  MEMBER(height);
  MEMBER(fps);
  MEMBER(format);
  ss << "}";
  return ss.str();
}

string hid_device_info::to_string() const {
  std::stringstream ss;
  ss << "{\n";
  MEMBER(id);
  MEMBER(vid);
  MEMBER(pid);
  MEMBER(unique_id);
  MEMBER(device_path);
  MEMBER(serial_number);
  ss << "}";
  return ss.str();
}

string usb_device_info::to_string() const {
  std::stringstream ss;
  ss << "{\n";
  MEMBER(id);
  MEMBER(vid);
  MEMBER(pid);
  MEMBER(mi);
  MEMBER(unique_id);
  MEMBER((uint16_t)conn_spec);
  ss << "}";
  return ss.str();
}

string uvc_device_info::to_string() const {
  std::stringstream ss;
  ss << "{\n";
  MEMBER(id);
  MEMBER(vid);
  MEMBER(pid);
  MEMBER(mi);
  MEMBER(unique_id);
  MEMBER(device_path);
  MEMBER((uint16_t)conn_spec);
  MEMBER(uvc_capabilities);
  MEMBER(has_metadata_node);
  MEMBER(metadata_node_id);
  ss << "}";
  return ss.str();
}

} // end librealuvc
