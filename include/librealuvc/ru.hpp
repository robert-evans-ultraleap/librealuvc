/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALUVC_RU_HPP
#define LIBREALUVC_RU_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace librealuvc {

using std::shared_ptr;
using std::string;
using std::vector;

// Forward declaration of classes

class backend;
class backend_device_group;
class command_transfer;
class device_changed_callback;
class device_watcher;
class extension_unit;
class hid_device;
class hid_device_info;
class notification;
class time_service;
class usb_device;
class usb_device_info;
class uvc_device;
class uvc_device_info;

struct extension_unit;
struct guid;

// backend supports querying devices and creating device-handles

class backend {
 public:
  virtual ~backend() = default;
  
  virtual shared_ptr<device_watcher> create_device_watcher() const = 0;  
  virtual shared_ptr<hid_device> create_hid_device(hid_device_info info) const = 0;
  virtual shared_ptr<usb_device> create_usb_device(usb_device_info info) const = 0;
  virtual shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const = 0;
  virtual shared_ptr<time_service> create_time_service() const = 0;
  
  virtual string get_device_serial(uint16_t vid, uint16_t pid, const std::string& uid) const {
    return "";
  }
  
  virtual vector<hid_device_info> query_hid_devices() const = 0;
  virtual vector<usb_device_info> query_usb_devices() const = 0;
  virtual vector<uvc_device_info> query_uvc_devices() const = 0;  
};

// create_backend() gives the appropriate backend for this platform

shared_ptr<backend> create_backend();

class backend_device_group {
 public:
  vector<hid_device_info> hid_devices;
  vector<usb_device_info> usb_devices;
  vector<uvc_device_info> uvc_devices;
 
 public:
  bool operator==(const backend_device_group& b) const;  
  operator string() const;
};

class command_transfer {
 public:
  virtual ~command_transfer() = default;

  virtual vector<uint8_t> send_receive(
    const vector<uint8_t>& data,
    int timeout_ms = 5000,
    bool require_response = true
  ) = 0;
};

typedef std::function<
  void(backend_device_group old_devs, backend_device_group new_devs)
> device_changed_callback;

class device_watcher {
 public:
  virtual ~device_watcher() = default;
  virtual void start(device_changed_callback callback) = 0;
  virtual void stop() = 0;
};

struct guid {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t data4[8];
};

// Host driver assigns subdevice and node
// unit and guid are hard-coded in firmware

struct extension_unit {
   int subdevice;
   int unit;
   int node;
   guid id;
};

enum power_state {
  D0, // full power
  D3  // sleep
};

class hid_device_info {
 public:
  string id;
  string vid;
  string pid;
  string unique_id;
  string device_path;
  string serial_number;
  
 public:
  bool operator==(const hid_device_info& b) const;  
  operator string() const;
};

struct hid_profile {
  string sensor_name;
  uint32_t frequency;
};

struct hid_sensor {
  string name;
};

#pragma pack(push, 1)
struct hid_sensor_data {
  int16_t x;
  char reserved1[2];
  int16_t y;
  char reserved2[2];
  int16_t z;
  char reserved3[2];
  uint32_t ts_low;
  uint32_t ts_high;
};
#pragma pack(pop)

struct hid_sensor_input {
  uint32_t index;
  string name;
};

struct sensor_data {
  hid_sensor sensor;
  frame_object fo;
};

enum custom_sensor_report_field {
  minimum,
  maximum,
  name,
  size,
  unit_expo,
  units,
  value
};

typedef std::function<void(const sensor_data&)> hid_callback;

class hid_device {
 public:
  virtual ~hid_device() = default;
  virtual void open(const vector<hid_profile>& hid_profiles) = 0;
  virtual void close() = 0;
  virtual void start_capture(hid_callback callback) = 0;
  virtual void stop_capture() = 0;
  virtual vector<hid_sensor> get_sensors() = 0;
  virtual vector<uint8_t> get_custom_report_data(
    const string& custom_sensor_name,
    const string& report_name,
    custom_sensor_report_field report_field
  ) = 0;
};

typedef std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> stream_profile_tuple;

class stream_profile {
 public:
  uint32_t width;
  uint32_t height;
  uint32_t fps;
  uint32_t format;
  
 public:
  bool operator==(const stream_profile& b);
  operator stream_profile_tuple() const;
};

class time_service {
 public:
  virtual ~time_service() = default;
  virtual double get_time() const = 0;
};

class usb_device : public command_transfer {
 public:
  // interrupt endpoint and any additional USB specific stuff
};

enum usb_spec : uint16_t {
  usb_undefined   = 0,
  usb1_type       = 0x0100,
  usb1_1_type     = 0x0110,
  usb2_type       = 0x0200,
  usb2_1_type     = 0x0210,
  usb3_type       = 0x0300,
  usb3_1_type     = 0x0310,
  usb3_2_type     = 0x0320,
};

class usb_spec_map {
 public:
  const string& operator[](usb_spec spec) const;
};

usb_spec_map usb_spec_names;

class usb_device_info {
 public:
  string id;
  uint16_t vid;
  uint16_t pid;
  uint16_t mi;
  string unique_id;
  usb_spec conn_spec;
 
 public:
  bool operator==(const usb_device_info& b) const;
  operator std::string() const;
};

class uvc_device {
 public:
  typedef std::function<void(const notification&)> error_callback;
 protected:
  error_callback _error_handler;
 public:
  virtual ~uvc_device() = default;
  
  virtual void probe_and_commit(stream_profile prof, frame_callback callback, int buffers = 4) = 0;
  virtual void stream_on(error_callback on_error = [](const notification&){}) = 0;
  virtual void start_callbacks() = 0;
  virtual void stop_callbacks() = 0;
  virtual void close(stream_profile profile) = 0;

  virtual void set_power_state(power_state state) = 0;
  virtual power_state get_power_state() const = 0;

  virtual void init_xu(const extension_unit& xu) = 0;
  virtual bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) = 0;
  virtual bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const = 0;
  virtual control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const = 0;

  virtual bool get_pu(rs2_option opt, int32_t& value) const = 0;
  virtual bool set_pu(rs2_option opt, int32_t value) = 0;
  virtual control_range get_pu_range(rs2_option opt) const = 0;

  virtual vector<stream_profile> get_profiles() const = 0;

  virtual void lock() const = 0;
  virtual void unlock() const = 0;

  virtual string get_device_location() const = 0;
  virtual usb_spec get_usb_specification() const = 0;
};

class uvc_device_info {
 public:
  string id; // distinguish different pins of one device
  uint16_t vid;
  uint16_t pid;
  uint16_t mi;
  string unique_id;
  usb_spec conn_spec;
  uint32_t uvc_capabilities;
  bool has_metadata_node;
  string metadata_node_id;
 
 public:
  uvc_device_info();
  ~uvc_device_info() = default;
  bool operator==(const usb_device_info& b) const;
  bool operator<(const uvc_device_info& b) const;
  operator std::string() const;
};

}

#endif
