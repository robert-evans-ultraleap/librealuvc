/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

// Workaround for missing python32_d.lib
#ifdef _DEBUG
# undef _DEBUG
# include <Python.h>
# define _DEBUG
#else
# include <Python.h>
#endif
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarrayobject.h>

#include "../src/backend.h"
#include "pybackend_extras.h"
#include "../../third-party/stb_image_write.h"

#include <cassert>
#include <cstdio>
#include <sstream>
#include <vector>

#if 0
#define D(fmt, ...) { \
  printf("DEBUG[%s,%d] ", __FILE__, __LINE__); \
  printf(fmt, __VA_ARGS__); \
  printf("\n"); \
  fflush(stdout); \
}
#else
#define D(fmt, ...) { }
#endif

#define NAME pyrealuvc
#define SNAME "pyrealuvc"

namespace py = pybind11;
using namespace pybind11::literals;

using namespace librealuvc;
using namespace pyrealuvc;

namespace {
  
// cv::Mat to/from Numpy array (adapted from opencv/modules/python/src2/cv2.cpp)

// We copy a good deal of code from opencv to try to make this work
// without needing to modify the OpenCV library.

using namespace cv;

class PyEnsureGIL {
 public:
  PyEnsureGIL() : _state(PyGILState_Ensure()) { }

  ~PyEnsureGIL() { PyGILState_Release(_state); }
 private:
  PyGILState_STATE _state;
};

class MyNumpyAllocator : public MatAllocator {
public:
  MyNumpyAllocator() { stdAllocator = Mat::getStdAllocator(); }
  ~MyNumpyAllocator() { }

  UMatData* x_allocate(PyObject* o, int dims, const int* sizes, int type, size_t* step) const {
    cv::UMatData* u = new cv::UMatData(this);
    u->data = u->origdata = (uchar*)PyArray_DATA((PyArrayObject*) o);
    npy_intp* _strides = PyArray_STRIDES((PyArrayObject*) o);
    for (int i = 0; i < dims-1; ++i)
      step[i] = (size_t)_strides[i];
    step[dims-1] = CV_ELEM_SIZE(type);
    u->size = sizes[0]*step[0];
    u->userdata = o;
    return u;
  }

  UMatData* allocate(
    int dims0, const int* sizes, int type,
    void* data, size_t* step, AccessFlag flags, UMatUsageFlags usageFlags) const CV_OVERRIDE {
    if (data != 0) {
      // issue #6969: CV_Error(Error::StsAssert, "The data should normally be NULL!");
      // probably this is safe to do in such extreme case
      return stdAllocator->allocate(dims0, sizes, type, data, step, flags, usageFlags);
    }
    PyEnsureGIL gil;

    int depth = CV_MAT_DEPTH(type);
    int cn = CV_MAT_CN(type);
    const int f = (int)(sizeof(size_t)/8);
    int typenum = depth == CV_8U ? NPY_UBYTE : depth == CV_8S ? NPY_BYTE :
    depth == CV_16U ? NPY_USHORT : depth == CV_16S ? NPY_SHORT :
    depth == CV_32S ? NPY_INT : depth == CV_32F ? NPY_FLOAT :
    depth == CV_64F ? NPY_DOUBLE : f*NPY_ULONGLONG + (f^1)*NPY_UINT;
    int i, dims = dims0;
    cv::AutoBuffer<npy_intp> _sizes(dims + 1);
    for (i = 0; i < dims; i++)
      _sizes[i] = sizes[i];
    if ( cn > 1 )
      _sizes[dims++] = cn;
    PyObject* o = PyArray_SimpleNew(dims, _sizes.data(), typenum);
    if (!o) {
      fprintf(stderr, "ERROR: The numpy array of typenum=%d, ndims=%d can not be created", typenum, dims);
    }
    auto u = this->x_allocate(o, dims0, sizes, type, step);
    u->userdata = o;
    return u;
  }

  bool allocate(UMatData* u, AccessFlag accessFlags, UMatUsageFlags usageFlags) const CV_OVERRIDE {
    return stdAllocator->allocate(u, accessFlags, usageFlags);
  }

  void deallocate(UMatData* u) const CV_OVERRIDE {
    if (!u)
      return;
    PyEnsureGIL gil;
    assert(u->urefcount >= 0);
    assert(u->refcount >= 0);
    if (u->refcount == 0) {
      PyObject* o = (PyObject*)u->userdata;
      Py_XDECREF(o);
      delete u;
    }
  }

  const MatAllocator* stdAllocator;
};

MyNumpyAllocator pybackend_numpy_alloc;

PyObject* pyopencv_from_mat(const Mat& m) {
  if (!m.data)
    Py_RETURN_NONE;
  Mat *p = (Mat*)&m;
  Mat temp;
  if (!p->u || (p->allocator != &pybackend_numpy_alloc)) {
    temp.allocator = &pybackend_numpy_alloc;
    m.copyTo(temp);
    p = &temp;
  }
  PyObject* o = (PyObject*)p->u->userdata;
  Py_INCREF(o);
  return o;
}

PyObject* pyopencv_from_bool(bool value) {
  return PyBool_FromLong(value);
}

} // anon

// Prevents expensive copies of pixel buffers into python
PYBIND11_MAKE_OPAQUE(std::vector<uint8_t>)

PYBIND11_MODULE(NAME, m) {
    if (_import_array() < 0) {
      fprintf(stderr, "FATAL: numpy.core.multiarray import fail");
      exit(1);
    }

#if 0
    py::enum_<librealuvc::usb_spec>(m, "USB_TYPE")
        .value("USB1", librealuvc::usb_spec::usb1_type)
        .value("USB1_1", librealuvc::usb_spec::usb1_1_type)
        .value("USB2", librealuvc::usb_spec::usb2_type)
        .value("USB2_1", librealuvc::usb_spec::usb2_1_type)
        .value("USB3", librealuvc::usb_spec::usb3_type)
        .value("USB3_1", librealuvc::usb_spec::usb3_1_type)
        .value("USB3_2", librealuvc::usb_spec::usb3_2_type);
#endif

    m.doc() = "Wrapper for librealuvc";

    py::class_<librealuvc::control_range> control_range(m, "control_range");
    control_range.def(py::init<>())
                 .def(py::init<int32_t, int32_t, int32_t, int32_t>(), "in_min"_a, "in_max"_a, "in_step"_a, "in_def"_a)
                 .def_readwrite("min", &librealuvc::control_range::min)
                 .def_readwrite("max", &librealuvc::control_range::max)
                 .def_readwrite("def", &librealuvc::control_range::def)
                 .def_readwrite("step", &librealuvc::control_range::step);

    py::class_<librealuvc::time_service> time_service(m, "time_service");
    time_service.def("get_time", &librealuvc::time_service::get_time);

#if 0
    py::class_<librealuvc::os_time_service, librealuvc::time_service> os_time_service(m, "os_time_service");
#endif

#define BIND_RAW_RO_ARRAY(class, name, type, size) #name, [](const class &c) -> const std::array<type, size>& { return reinterpret_cast<const std::array<type, size>&>(c.name); }
#define BIND_RAW_RW_ARRAY(class, name, type, size) BIND_RAW_RO_ARRAY(class, name, type, size), [](class &c, const std::array<type, size> &arr) { for (int i=0; i<size; ++i) c.name[i] = arr[i]; }

    py::class_<librealuvc::guid> guid(m, "guid");
    guid.def_readwrite("data1", &librealuvc::guid::data1)
        .def_readwrite("data2", &librealuvc::guid::data2)
        .def_readwrite("data3", &librealuvc::guid::data3)
        .def_property(BIND_RAW_RW_ARRAY(librealuvc::guid, data4, uint8_t, 8))
        .def("__init__", [](librealuvc::guid &g, uint32_t d1, uint32_t d2, uint32_t d3, std::array<uint8_t, 8> d4)
            {
                new (&g) librealuvc::guid();
                g.data1 = d1;
                g.data2 = d2;
                g.data3 = d3;
                for (int i=0; i<8; ++i) g.data4[i] = d4[i];
            }, "data1"_a, "data2"_a, "data3"_a, "data4"_a)
        .def("__init__", [](librealuvc::guid &g, const std::string &str)
            {
                new (&g) librealuvc::guid(stoguid(str));
            });

    py::class_<librealuvc::extension_unit> extension_unit(m, "extension_unit");
    extension_unit.def(py::init<>())
                  .def("__init__", [](librealuvc::extension_unit & xu, int s, int u, int n, librealuvc::guid g)
                      {
                          new (&xu) librealuvc::extension_unit { s, u, n, g };
                      }, "subdevice"_a, "unit"_a, "node"_a, "guid"_a)
                  .def_readwrite("subdevice", &librealuvc::extension_unit::subdevice)
                  .def_readwrite("unit", &librealuvc::extension_unit::unit)
                  .def_readwrite("node", &librealuvc::extension_unit::node)
                  .def_readwrite("id", &librealuvc::extension_unit::id);

    py::class_<librealuvc::command_transfer, std::shared_ptr<librealuvc::command_transfer>> command_transfer(m, "command_transfer");
    command_transfer.def("send_receive", &librealuvc::command_transfer::send_receive, "data"_a, "timeout_ms"_a=5000, "require_response"_a=true);

#if 0
    py::enum_<rs2_option> option(m, "option");
    option.value("backlight_compensation", RS2_OPTION_BACKLIGHT_COMPENSATION)
        .value("brightness", RS2_OPTION_BRIGHTNESS)
        .value("contrast", RS2_OPTION_CONTRAST)
        .value("exposure", RS2_OPTION_EXPOSURE)
        .value("gain", RS2_OPTION_GAIN)
        .value("gamma", RS2_OPTION_GAMMA)
        .value("hue", RS2_OPTION_HUE)
        .value("saturation", RS2_OPTION_SATURATION)
        .value("sharpness", RS2_OPTION_SHARPNESS)
        .value("white_balance", RS2_OPTION_WHITE_BALANCE)
        .value("enable_auto_exposure", RS2_OPTION_ENABLE_AUTO_EXPOSURE)
        .value("enable_auto_white_balance", RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)
        .value("visual_preset", RS2_OPTION_VISUAL_PRESET)
        .value("laser_power", RS2_OPTION_LASER_POWER)
        .value("accuracy", RS2_OPTION_ACCURACY)
        .value("motion_range", RS2_OPTION_MOTION_RANGE)
        .value("filter_option", RS2_OPTION_FILTER_OPTION)
        .value("confidence_threshold", RS2_OPTION_CONFIDENCE_THRESHOLD)
        .value("emitter_enabled", RS2_OPTION_EMITTER_ENABLED)
        .value("frames_queue_size", RS2_OPTION_FRAMES_QUEUE_SIZE)
        .value("total_frame_drops", RS2_OPTION_TOTAL_FRAME_DROPS)
        .value("auto_exposure_mode", RS2_OPTION_AUTO_EXPOSURE_MODE)
        .value("power_line_frequency", RS2_OPTION_POWER_LINE_FREQUENCY)
        .value("asic_temperature", RS2_OPTION_ASIC_TEMPERATURE)
        .value("error_polling_enabled", RS2_OPTION_ERROR_POLLING_ENABLED)
        .value("projector_temperature", RS2_OPTION_PROJECTOR_TEMPERATURE)
        .value("output_trigger_enabled", RS2_OPTION_OUTPUT_TRIGGER_ENABLED)
        .value("motion_module_temperature", RS2_OPTION_MOTION_MODULE_TEMPERATURE)
        .value("depth_units", RS2_OPTION_DEPTH_UNITS)
        .value("enable_motion_correction", RS2_OPTION_ENABLE_MOTION_CORRECTION)
        .value("auto_exposure_priority", RS2_OPTION_AUTO_EXPOSURE_PRIORITY)
        .value("color_scheme", RS2_OPTION_COLOR_SCHEME)
        .value("histogram_equalization_enabled", RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED)
        .value("min_distance", RS2_OPTION_MIN_DISTANCE)
        .value("max_distance", RS2_OPTION_MAX_DISTANCE)
        .value("texture_source", RS2_OPTION_TEXTURE_SOURCE)
        .value("filter_magnitude", RS2_OPTION_FILTER_MAGNITUDE)
        .value("filter_smooth_alpha", RS2_OPTION_FILTER_SMOOTH_ALPHA)
        .value("filter_smooth_delta", RS2_OPTION_FILTER_SMOOTH_DELTA)
        .value("filter_holes_fill", RS2_OPTION_HOLES_FILL)
        .value("stereo_baseline", RS2_OPTION_STEREO_BASELINE)
        .value("count", RS2_OPTION_COUNT);
#endif

    py::enum_<librealuvc::power_state> power_state(m, "power_state");
    power_state.value("D0", librealuvc::power_state::D0)
               .value("D3", librealuvc::power_state::D3);
    power_state.export_values();

    py::class_<librealuvc::stream_profile> stream_profile(m, "stream_profile");
    stream_profile.def_readwrite("width", &librealuvc::stream_profile::width)
                  .def_readwrite("height", &librealuvc::stream_profile::height)
                  .def_readwrite("fps", &librealuvc::stream_profile::fps)
                  .def_readwrite("format", &librealuvc::stream_profile::format)
//                  .def("stream_profile_tuple", &librealuvc::stream_profile::stream_profile_tuple) // converstion operator to std::tuple
                  .def(py::self == py::self).def("__repr__", [](const librealuvc::stream_profile &p) {
        std::stringstream ss;
        ss << "<" SNAME ".stream_profile: "
            << p.width
            << "x" << p.height << " @ " << p.fps << "fps "
            << std::hex << p.format << ">";
        return ss.str();
    });;

    // Bind std::vector<uint8_t> to act like a pythonic list
    py::bind_vector<std::vector<uint8_t>>(m, "VectorByte");

    py::class_<librealuvc::frame_object> frame_object(m, "frame_object");
    frame_object.def_readwrite("frame_size", &librealuvc::frame_object::frame_size)
                .def_readwrite("metadata_size", &librealuvc::frame_object::metadata_size)
                .def_property_readonly("pixels", [](const librealuvc::frame_object &f) { return std::vector<uint8_t>(static_cast<const uint8_t*>(f.pixels), static_cast<const uint8_t*>(f.pixels)+f.frame_size);})
                .def_property_readonly("metadata", [](const librealuvc::frame_object &f) { return std::vector<uint8_t>(static_cast<const uint8_t*>(f.metadata), static_cast<const uint8_t*>(f.metadata)+f.metadata_size);})
                .def("save_png", [](const librealuvc::frame_object &f, std::string fn, int w, int h, int bpp, int s)
                    {
                        stbi_write_png(fn.c_str(), w, h, bpp, f.pixels, s);
                    }, "filename"_a, "width"_a, "height"_a, "bytes_per_pixel"_a, "stride"_a)
                .def("save_png", [](const librealuvc::frame_object &f, std::string fn, int w, int h, int bpp)
                    {
                        stbi_write_png(fn.c_str(), w, h, bpp, f.pixels, w*bpp);
                    }, "filename"_a, "width"_a, "height"_a, "bytes_per_pixel"_a);

    py::class_<librealuvc::uvc_device_info> uvc_device_info(m, "uvc_device_info");
    uvc_device_info.def_readwrite("id", &librealuvc::uvc_device_info::id, "To distinguish between different pins of the same device.")
                   .def_readwrite("vid", &librealuvc::uvc_device_info::vid)
                   .def_readwrite("pid", &librealuvc::uvc_device_info::pid)
                   .def_readwrite("mi", &librealuvc::uvc_device_info::mi)
                   .def_readwrite("unique_id", &librealuvc::uvc_device_info::unique_id)
                   .def_readwrite("device_path", &librealuvc::uvc_device_info::device_path)
                   .def(py::self == py::self);

    py::class_<librealuvc::usb_device_info> usb_device_info(m, "usb_device_info");
    usb_device_info.def_readwrite("id", &librealuvc::usb_device_info::id)
                   .def_readwrite("vid", &librealuvc::usb_device_info::vid)
                   .def_readwrite("pid", &librealuvc::usb_device_info::pid)
                   .def_readwrite("mi", &librealuvc::usb_device_info::mi)
                   .def_readwrite("unique_id", &librealuvc::usb_device_info::unique_id);

    py::class_<librealuvc::hid_device_info> hid_device_info(m, "hid_device_info");
    hid_device_info.def_readwrite("id", &librealuvc::hid_device_info::id)
                   .def_readwrite("vid", &librealuvc::hid_device_info::vid)
                   .def_readwrite("pid", &librealuvc::hid_device_info::pid)
                   .def_readwrite("unique_id", &librealuvc::hid_device_info::unique_id)
                   .def_readwrite("device_path", &librealuvc::hid_device_info::device_path);

    py::class_<librealuvc::hid_sensor> hid_sensor(m, "hid_sensor");
    hid_sensor.def_readwrite("name", &librealuvc::hid_sensor::name);

    py::class_<librealuvc::hid_sensor_input> hid_sensor_input(m, "hid_sensor_input");
    hid_sensor_input.def_readwrite("index", &librealuvc::hid_sensor_input::index)
                    .def_readwrite("name", &librealuvc::hid_sensor_input::name);
#if 0
    py::class_<librealuvc::callback_data> callback_data(m, "callback_data");
    callback_data.def_readwrite("sensor", &librealuvc::callback_data::sensor)
                 .def_readwrite("sensor_input", &librealuvc::callback_data::sensor_input)
                 .def_readwrite("value", &librealuvc::callback_data::value);
#endif

#if 0
    py::class_<librealuvc::sensor_data> sensor_data(m, "sensor_data");
    sensor_data.def_readwrite("sensor", &librealuvc::sensor_data::sensor)
               .def_readwrite("fo", &librealuvc::sensor_data::fo);
#endif

    py::class_<librealuvc::hid_profile> hid_profile(m, "hid_profile");
    hid_profile.def(py::init<>())
               .def_readwrite("sensor_name", &librealuvc::hid_profile::sensor_name)
               .def_readwrite("frequency", &librealuvc::hid_profile::frequency);

    py::enum_<librealuvc::custom_sensor_report_field> custom_sensor_report_field(m, "custom_sensor_report_field");
    custom_sensor_report_field.value("minimum", librealuvc::custom_sensor_report_field::minimum)
                              .value("maximum", librealuvc::custom_sensor_report_field::maximum)
                              .value("name", librealuvc::custom_sensor_report_field::name)
                              .value("size", librealuvc::custom_sensor_report_field::size)
                              .value("unit_expo", librealuvc::custom_sensor_report_field::unit_expo)
                              .value("units", librealuvc::custom_sensor_report_field::units)
                              .value("value", librealuvc::custom_sensor_report_field::value);
    custom_sensor_report_field.export_values();

    py::class_<librealuvc::hid_sensor_data> hid_sensor_data(m, "hid_sensor_data");
    hid_sensor_data.def_readwrite("x", &librealuvc::hid_sensor_data::x)
                   .def_property(BIND_RAW_RW_ARRAY(librealuvc::hid_sensor_data, reserved1, char, 2))
                   .def_readwrite("y", &librealuvc::hid_sensor_data::y)
                   .def_property(BIND_RAW_RW_ARRAY(librealuvc::hid_sensor_data, reserved2, char, 2))
                   .def_readwrite("z", &librealuvc::hid_sensor_data::z)
                   .def_property(BIND_RAW_RW_ARRAY(librealuvc::hid_sensor_data, reserved3, char, 2))
                   .def_readwrite("ts_low", &librealuvc::hid_sensor_data::ts_low)
                   .def_readwrite("ts_high", &librealuvc::hid_sensor_data::ts_high);

    py::class_<librealuvc::hid_device, std::shared_ptr<librealuvc::hid_device>> hid_device(m, "hid_device");

    hid_device.def("open", &librealuvc::hid_device::open, "hid_profiles"_a)
              .def("close", &librealuvc::hid_device::close)
              .def("stop_capture", &librealuvc::hid_device::stop_capture)
              .def("start_capture", &librealuvc::hid_device::start_capture, "callback"_a)
              .def("get_sensors", &librealuvc::hid_device::get_sensors)
              .def("get_custom_report_data", &librealuvc::hid_device::get_custom_report_data,
                   "custom_sensor_name"_a, "report_name"_a, "report_field"_a);

#if 0
    py::class_<librealuvc::multi_pins_hid_device, std::shared_ptr<librealuvc::multi_pins_hid_device>, librealuvc::hid_device> multi_pins_hid_device(m, "multi_pins_hid_device");
    multi_pins_hid_device.def(py::init<std::vector<std::shared_ptr<librealuvc::hid_device>>&>())
                         .def("open", &librealuvc::multi_pins_hid_device::open, "hid_profiles"_a)
                         .def("close", &librealuvc::multi_pins_hid_device::close)
                         .def("stop_capture", &librealuvc::multi_pins_hid_device::stop_capture)
                         .def("start_capture", &librealuvc::multi_pins_hid_device::start_capture, "callback"_a)
                         .def("get_sensors", &librealuvc::multi_pins_hid_device::get_sensors)
                         .def("get_custom_report_data", &librealuvc::multi_pins_hid_device::get_custom_report_data,
                              "custom_sensor_name"_a, "report_name"_a, "report_field"_a);
#endif

    py::class_<librealuvc::uvc_device, std::shared_ptr<librealuvc::uvc_device>> uvc_device(m, "uvc_device");
#if 0
    py::class_<librealuvc::retry_controls_work_around, std::shared_ptr<librealuvc::retry_controls_work_around>, librealuvc::uvc_device> retry_controls_work_around(m, "retry_controls_work_around");

    retry_controls_work_around.def(py::init<std::shared_ptr<librealuvc::uvc_device>>())
        .def("probe_and_commit",
            [](librealuvc::retry_controls_work_around& dev, const librealuvc::stream_profile& profile,
                std::function<void(librealuvc::frame_object)> callback) {
        dev.probe_and_commit(profile, [=](librealuvc::stream_profile p,
            librealuvc::frame_object fo, std::function<void()> next)
        {
            callback(fo);
            next();
        }, 4);
            }
            , "profile"_a, "callback"_a)
        .def("stream_on", [](librealuvc::retry_controls_work_around& dev) {
                dev.stream_on([](const notification& n)
                {
                });
            })
        .def("start_callbacks", &librealuvc::retry_controls_work_around::start_callbacks)
        .def("stop_callbacks", &librealuvc::retry_controls_work_around::stop_callbacks)
        .def("get_usb_specification", &librealuvc::retry_controls_work_around::get_usb_specification)
        .def("close", [](librealuvc::retry_controls_work_around &dev, librealuvc::stream_profile profile)
            {
                py::gil_scoped_release release;
                dev.close(profile);
            }, "profile"_a)
        .def("set_power_state", &librealuvc::retry_controls_work_around::set_power_state, "state"_a)
        .def("get_power_state", &librealuvc::retry_controls_work_around::get_power_state)
        .def("init_xu", &librealuvc::retry_controls_work_around::init_xu, "xu"_a)
        .def("set_xu", [](librealuvc::retry_controls_work_around &dev, const librealuvc::extension_unit &xu, uint8_t ctrl, py::list l)
            {
                std::vector<uint8_t> data(l.size());
                for (int i = 0; i < l.size(); ++i)
                    data[i] = l[i].cast<uint8_t>();
                return dev.set_xu(xu, ctrl, data.data(), (int)data.size());
            }, "xu"_a, "ctrl"_a, "data"_a)
        .def("set_xu", [](librealuvc::retry_controls_work_around &dev, const librealuvc::extension_unit &xu, uint8_t ctrl, std::vector<uint8_t> &data)
            {
                return dev.set_xu(xu, ctrl, data.data(), (int)data.size());
            }, "xu"_a, "ctrl"_a, "data"_a)
        .def("get_xu", [](const librealuvc::retry_controls_work_around &dev, const librealuvc::extension_unit &xu, uint8_t ctrl, size_t len)
            {
                std::vector<uint8_t> data(len);
                dev.get_xu(xu, ctrl, data.data(), (int)len);
                py::list ret(len);
                for (size_t i = 0; i < len; ++i)
                    ret[i] = data[i];
                return ret;
            }, "xu"_a, "ctrl"_a, "len"_a)
        .def("get_xu_range", &librealuvc::retry_controls_work_around::get_xu_range, "xu"_a, "ctrl"_a, "len"_a)
        .def("get_pu", [](librealuvc::retry_controls_work_around& dev, rs2_option opt) {
                int val = 0;
                dev.get_pu(opt, val);
                return val;
            }, "opt"_a)
        .def("set_pu", &librealuvc::retry_controls_work_around::set_pu, "opt"_a, "value"_a)
        .def("get_pu_range", &librealuvc::retry_controls_work_around::get_pu_range, "opt"_a)
        .def("get_profiles", &librealuvc::retry_controls_work_around::get_profiles)
        .def("lock", &librealuvc::retry_controls_work_around::lock)
        .def("unlock", &librealuvc::retry_controls_work_around::unlock)
        .def("get_device_location", &librealuvc::retry_controls_work_around::get_device_location);
#endif
    py::class_<librealuvc::usb_device, librealuvc::command_transfer, std::shared_ptr<librealuvc::usb_device>> usb_device(m, "usb_device");

    py::class_<librealuvc::backend, std::shared_ptr<librealuvc::backend>> backend(m, "backend");
    backend.def("create_uvc_device", &librealuvc::backend::create_uvc_device, "info"_a)
        .def("query_uvc_devices", &librealuvc::backend::query_uvc_devices)
        .def("create_usb_device", &librealuvc::backend::create_usb_device, "info"_a)
        .def("query_usb_devices", &librealuvc::backend::query_usb_devices)
        .def("create_hid_device", &librealuvc::backend::create_hid_device, "info"_a)
        .def("query_hid_devices", &librealuvc::backend::query_hid_devices)
        .def("create_time_service", &librealuvc::backend::create_time_service);
    
    py::class_<librealuvc::VideoCapture, std::shared_ptr<librealuvc::VideoCapture>> vidcap(m, "VideoCapture");
    vidcap
      .def(py::init<>())
      .def(py::init<int>())
      .def(py::init<const cv::String&>())
      .def(py::init<const cv::String&, int>())
      .def("get",      &librealuvc::VideoCapture::get, "propId"_a)
       // .def("grab",     &librealuvc::VideoCapture::grab)
      .def("isOpened", &librealuvc::VideoCapture::isOpened)
      .def("read",
        [](librealuvc::VideoCapture& this_ref) {
          auto& image = this_ref.get_reusable_image();
          //cv::Mat image;
          bool ok = this_ref.read(image);
          PyObject* image_obj = pyopencv_from_mat(image);
          auto tuple = Py_BuildValue("(NN)", pyopencv_from_bool(ok), image_obj);
          auto result = py::reinterpret_steal<py::object>(tuple);
          return result;
        }
      )
      .def("release",  &librealuvc::VideoCapture::release)
      // .def("retrieve", &librealuvc::VideoCapture::retrieve, "image"_a, "flag"_a)
      .def("set",      &librealuvc::VideoCapture::set, "propId"_a, "value"_a)
      .def("is_extended",    &librealuvc::VideoCapture::is_extended)
      .def("get_vendor_id",  &librealuvc::VideoCapture::get_vendor_id)
      .def("get_product_id", &librealuvc::VideoCapture::get_product_id);

#if 0
    py::class_<librealuvc::multi_pins_uvc_device, std::shared_ptr<librealuvc::multi_pins_uvc_device>, librealuvc::uvc_device> multi_pins_uvc_device(m, "multi_pins_uvc_device");
    multi_pins_uvc_device.def(py::init<std::vector<std::shared_ptr<librealuvc::uvc_device>>&>())
        .def("probe_and_commit",
            [](librealuvc::multi_pins_uvc_device& dev, const librealuvc::stream_profile& profile,
                std::function<void(librealuvc::frame_object)> callback) {
        dev.probe_and_commit(profile, [=](librealuvc::stream_profile p,
            librealuvc::frame_object fo, std::function<void()> next)
        {
            callback(fo);
            next();
        }, 4);
    }
            , "profile"_a, "callback"_a)
        .def("stream_on", [](librealuvc::multi_pins_uvc_device& dev) {
        dev.stream_on([](const notification& n)
        {
        });
    })
        .def("start_callbacks", &librealuvc::multi_pins_uvc_device::start_callbacks)
        .def("stop_callbacks", &librealuvc::multi_pins_uvc_device::stop_callbacks)
        .def("get_usb_specification", &librealuvc::multi_pins_uvc_device::get_usb_specification)
        .def("close", [](librealuvc::multi_pins_uvc_device &dev, librealuvc::stream_profile profile)
    {
        py::gil_scoped_release release;
        dev.close(profile);
    }, "profile"_a)
        .def("set_power_state", &librealuvc::multi_pins_uvc_device::set_power_state, "state"_a)
        .def("get_power_state", &librealuvc::multi_pins_uvc_device::get_power_state)
        .def("init_xu", &librealuvc::multi_pins_uvc_device::init_xu, "xu"_a)
        .def("set_xu", [](librealuvc::multi_pins_uvc_device &dev, const librealuvc::extension_unit &xu, uint8_t ctrl, py::list l)
    {
        std::vector<uint8_t> data(l.size());
        for (int i = 0; i < l.size(); ++i)
            data[i] = l[i].cast<uint8_t>();
        return dev.set_xu(xu, ctrl, data.data(), (int)data.size());
    }, "xu"_a, "ctrl"_a, "data"_a)
        .def("set_xu", [](librealuvc::multi_pins_uvc_device &dev, const librealuvc::extension_unit &xu, uint8_t ctrl, std::vector<uint8_t> &data)
    {
        return dev.set_xu(xu, ctrl, data.data(), (int)data.size());
    }, "xu"_a, "ctrl"_a, "data"_a)
        .def("get_xu", [](const librealuvc::multi_pins_uvc_device &dev, const librealuvc::extension_unit &xu, uint8_t ctrl, size_t len)
    {
        std::vector<uint8_t> data(len);
        dev.get_xu(xu, ctrl, data.data(), (int)len);
        py::list ret(len);
        for (size_t i = 0; i < len; ++i)
            ret[i] = data[i];
        return ret;
    }, "xu"_a, "ctrl"_a, "len"_a)
        .def("get_xu_range", &librealuvc::multi_pins_uvc_device::get_xu_range, "xu"_a, "ctrl"_a, "len"_a)
        .def("get_pu", [](librealuvc::multi_pins_uvc_device& dev, rs2_option opt) {
        int val = 0;
        dev.get_pu(opt, val);
        return val;
    }, "opt"_a)
        .def("set_pu", &librealuvc::multi_pins_uvc_device::set_pu, "opt"_a, "value"_a)
        .def("get_pu_range", &librealuvc::multi_pins_uvc_device::get_pu_range, "opt"_a)
        .def("get_profiles", &librealuvc::multi_pins_uvc_device::get_profiles)
        .def("lock", &librealuvc::multi_pins_uvc_device::lock)
        .def("unlock", &librealuvc::multi_pins_uvc_device::unlock)
        .def("get_device_location", &librealuvc::multi_pins_uvc_device::get_device_location);
#endif

    /*py::enum_<command> command_py(m, "command");
    command_py.value("enable_advanced_mode", command::enable_advanced_mode)
              .value("advanced_mode_enabled", command::advanced_mode_enabled)
              .value("reset", command::reset)
              .value("set_advanced", command::set_advanced)
              .value("get_advanced", command::get_advanced);*/

    m.def("create_backend", &librealuvc::create_backend, py::return_value_policy::move);
    m.def("encode_command", [](uint8_t opcode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4, py::list l)
        {
            std::vector<uint8_t> data(l.size());
            for (int i = 0; i < l.size(); ++i)
                data[i] = l[i].cast<uint8_t>();
            return encode_command(static_cast<command>(opcode), p1, p2, p3, p4, data);
        }, "opcode"_a, "p1"_a=0, "p2"_a=0, "p3"_a=0, "p4"_a=0, "data"_a = py::list(0));
}
