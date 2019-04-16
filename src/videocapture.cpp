// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include <librealuvc/ru_videocapture.h>
#include <librealuvc/ru_uvc.h>
#include <librealuvc/realuvc.h>
#include <opencv2/core/mat.hpp>
#include <librealuvc/realuvc_driver.h>
#include <chrono>
#include <exception>
#include <mutex>
#include <thread>

#if 0
#define D(...) { printf("DEBUG[%s,%d] ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define D(...) { }
#endif

namespace librealuvc {

namespace {

uint32_t str2fourcc(const char* s) {
  uint32_t result = 0;
  for (int j = 0; j < 4; ++j) {
    uint32_t b = ((int)s[j] & 0xff);
    result |= (b << 8*(3-j));
  }
  return result;
}

const char* prop_name(int prop_id) {
  switch (prop_id) {
#define PROP(x) case cv::CAP_PROP_##x: return "CAP_PROP_" #x;
    PROP(AUTO_EXPOSURE)
    PROP(AUTO_WB)
    PROP(AUTOFOCUS)
    PROP(BACKEND)
    PROP(BACKLIGHT)
    PROP(BRIGHTNESS)
    PROP(BUFFERSIZE)
    PROP(CHANNEL)
    PROP(CONTRAST)
    PROP(CONVERT_RGB)
    PROP(EXPOSURE)
    PROP(FOCUS)
    PROP(FORMAT)
    PROP(FOURCC)
    PROP(FPS)
    PROP(FRAME_COUNT)
    PROP(FRAME_HEIGHT)
    PROP(FRAME_WIDTH)
    PROP(GAIN)
    PROP(GAMMA)
    PROP(GUID)
    PROP(HUE)
    PROP(IRIS)
    PROP(ISO_SPEED)
    PROP(MODE)
    PROP(MONOCHROME)
    PROP(PAN)
    PROP(POS_AVI_RATIO)
    PROP(POS_FRAMES)
    PROP(POS_MSEC)
    PROP(RECTIFICATION)
    PROP(ROLL)
    PROP(SAR_DEN)
    PROP(SAR_NUM)
    PROP(SATURATION)
    PROP(SETTINGS)
    PROP(SHARPNESS)
    PROP(TEMPERATURE)
    PROP(TILT)
    PROP(TRIGGER)
    PROP(TRIGGER_DELAY)
    PROP(WB_TEMPERATURE)
    PROP(WHITE_BALANCE_BLUE_U)
    PROP(WHITE_BALANCE_RED_V)
    PROP(ZOOM)
#undef PRO
    default:
      return("UNKNOWN");
  }
}

// A realuvc-managed device will pass frame buffers by callback. These will
// wrapped in a cv::Mat and queued until a VideoCapture::read(), 
// VideoCapture::retrieve(), or being dropped due to queue overflow.

} // end anon

class VideoStream : public IVideoStream {
 public:
  std::mutex mutex_;
  stream_profile profile_;
  bool is_streaming_;
  DevFrameQueue queue_;
  
 public:
  VideoStream(DevFrameFixup fixup, int max_size = 1) :
    is_streaming_(false),
    queue_(fixup, max_size) {
    profile_.width = 640;
    profile_.height = 480;
    profile_.fps = 30;
    profile_.format = str2fourcc("YUY2" /* "I420" */);
  }

  virtual ~VideoStream() { }
};

VideoCapture::VideoCapture() :
  is_opencv_(false),
  is_realuvc_(false) {
}

VideoCapture::VideoCapture(int index) :
  is_opencv_(false),
  is_realuvc_(false) {
  this->open(index);
}

VideoCapture::VideoCapture(const cv::String& filename) :
  is_opencv_(false),
  is_realuvc_(false) {
  this->open(filename);
}

VideoCapture::VideoCapture(const cv::String& filename, int api_preference) :
  is_opencv_(false),
  is_realuvc_(false) {
  this->open(filename, api_preference);
}

VideoCapture::~VideoCapture() {
  release();
}

static double get_pu(const std::shared_ptr<uvc_device>& dev, ru_option opt) {
  int32_t val = 0;
  bool ok = dev->get_pu(opt, val);
  if (!ok) val = 0;
  return (double)val;
}

double VideoCapture::get(int prop_id) const {
  try {
  if (is_opencv_) return opencv_->get(prop_id);
  if (!is_realuvc_) return 0.0;
  auto istream = std::dynamic_pointer_cast<VideoStream>(istream_);
  std::unique_lock<std::mutex> lock(istream->mutex_);
  //printf("DEBUG: VideoCapture::get(%s) ...\n", prop_name(prop_id)); fflush(stdout);
  switch (prop_id) {
    // properties which we can handle
    case cv::CAP_PROP_BRIGHTNESS:
      return get_pu(realuvc_, RU_OPTION_BRIGHTNESS);
    case cv::CAP_PROP_CONTRAST:
      return get_pu(realuvc_, RU_OPTION_CONTRAST);
    case cv::CAP_PROP_CONVERT_RGB:
      // FIXME
      return true;
    case cv::CAP_PROP_FOURCC:
      return (double)istream->profile_.format;
    case cv::CAP_PROP_FPS:
      return (double)istream->profile_.fps;
    case cv::CAP_PROP_FRAME_HEIGHT:
      return (double)istream->profile_.height;
    case cv::CAP_PROP_FRAME_WIDTH:
      return (double)istream->profile_.width;
    case cv::CAP_PROP_GAIN:
      return get_pu(realuvc_, RU_OPTION_GAIN);
    case cv::CAP_PROP_GAMMA:
      return get_pu(realuvc_, RU_OPTION_GAMMA);
    case cv::CAP_PROP_SATURATION:
      return get_pu(realuvc_, RU_OPTION_SATURATION);
    case cv::CAP_PROP_SHARPNESS:
      return get_pu(realuvc_, RU_OPTION_SHARPNESS);
    case cv::CAP_PROP_ZOOM:
      return get_pu(realuvc_, RU_OPTION_ZOOM_ABSOLUTE);
    // properties we will silently ignore
    case cv::CAP_PROP_POS_MSEC:
    case cv::CAP_PROP_POS_FRAMES:
    case cv::CAP_PROP_POS_AVI_RATIO:
    case cv::CAP_PROP_FRAME_COUNT:
    case cv::CAP_PROP_FORMAT:
    case cv::CAP_PROP_MODE:
    case cv::CAP_PROP_HUE:
      return 0.0;
    // invalid properties
    default:
      return 0.0;
  }
  } catch (std::exception& e) {
    printf("EXCEPTION: VideoCapture::get %s\n", e.what());
    throw;
  }
  return 0.0;
}

bool VideoCapture::grab() {
  if (is_opencv_) return opencv_->grab();
  // FIXME: we should grab the image here for later retrieve()
  return true;
}

bool VideoCapture::isOpened() const {
  if (is_opencv_) return opencv_->isOpened();
  return is_realuvc_;
}

// The images from Leap Motion devices are encoded in non-standard
// ways, but we'll fix them up in realuvc_driver.cpp

bool VideoCapture::open(int index) {
  auto backend = create_backend();
  auto info = backend->query_uvc_devices();
  is_realuvc_ = false;
  realuvc_.reset();
  if ((index < 0) || (index >= (int)info.size())) {
    return false;
  }
  realuvc_ = backend->create_uvc_device(info[index]);
  if (!realuvc_) {
    return false;
  }
  is_realuvc_ = true;
  vendor_id_ = info[index].vid;
  product_id_ = info[index].pid;
  // Kludge for the weird frame formats returned by Leap Peripheral/Rigel
  DevFrameFixup fixup = FIXUP_NORMAL;
  if ((vendor_id_ == 0xf182) && (product_id_ == 0x0003)) { // Leap Peripheral
    fixup = FIXUP_GRAY8_PIX_L_PIX_R;
  } else if( (vendor_id_ == 0x2936) && (product_id_ == 0x1202)) { // Leap Rigel
    fixup = FIXUP_GRAY8_ROW_L_ROW_R;
  }
  // Set low-power sleep state
  realuvc_->set_power_state(D3);
  // Wait a while
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  // Set full-power state before using the device
  realuvc_->set_power_state(D0);
  istream_ = std::make_shared<VideoStream>(fixup);
  return true;
}

bool VideoCapture::open(const cv::String& filename) {
  if (is_realuvc_) {
    is_realuvc_ = false;
    realuvc_.reset();
  }
  is_opencv_ = true;
  opencv_.reset(new cv::VideoCapture());
  return opencv_->open(filename);
}

bool VideoCapture::open(const cv::String& filename, int api_preference) {
  if (is_realuvc_) {
    is_realuvc_ = false;
    realuvc_.reset();
  }
  is_opencv_ = true;
  opencv_.reset(new cv::VideoCapture());
  return opencv_->open(filename, api_preference);
}

VideoCapture& VideoCapture::operator>>(cv::Mat& image) {
  if (is_opencv_) { (*opencv_) >> image; return *this; }
  read(image);
  return *this;
}

VideoCapture& VideoCapture::operator>>(cv::UMat& image) {
  if (is_opencv_) { (*opencv_) >> image; return *this; }
  //read(image);
  assert(0);
  return *this;
}

bool VideoCapture::read(cv::OutputArray image) {
  try {
  if (is_opencv_) return opencv_->read(image);
  if (!is_realuvc_) return false;
  auto istream = std::dynamic_pointer_cast<VideoStream>(istream_);
  { std::unique_lock<std::mutex> lock(istream->mutex_);
    if (!istream->is_streaming_) {
      D("profile width %d, height %d, fps %d, format 0x%x",
        istream->profile_.width, istream->profile_.height,
        istream->profile_.fps, istream->profile_.format);
      D("probe_and_commit() ...");
      auto captured_istream = istream;
      realuvc_->probe_and_commit(
        istream->profile_,
        [captured_istream](stream_profile profile, frame_object frame, std::function<void()> func) {
          captured_istream->queue_.push_back(profile, frame, func);
        },
        4
      );

      try {
        D("stream_on() ...");
        realuvc_->stream_on();
        D("start_callbacks() ...");
        realuvc_->start_callbacks();
        istream->is_streaming_ = true;
      } catch (std::exception e) {
        printf("ERROR: caught exception %s\n", e.what());
        fflush(stdout);      
      }
    }
    if (!istream->is_streaming_) return false;
  } // don't hold the mutex while possibly waiting for frame
  cv::Mat tmp;
  istream->queue_.pop_front(tmp); // wait for a frame if necessary
  if (image.needed()) {
    // OutputArray::assign() will not copy unless it needs to
    image.assign(tmp);
  }
  } catch (std::exception& e) {
    printf("EXCEPTION: VideoCapture::read %s\n", e.what());
    throw;
  }

  return true;
}

void VideoCapture::release() {
  D("VideoCapture::release() ...");
  if (is_opencv_) {
    opencv_->release();
    opencv_.reset();
  }
  if (is_realuvc_) {
    auto istream = std::dynamic_pointer_cast<VideoStream>(istream_);
    if (istream) { 
      D("lock istream->mutex_ ...");
      std::unique_lock<std::mutex> lock(istream->mutex_);
      if (istream->is_streaming_) {
        D("stop_callbacks()");
        realuvc_->stop_callbacks();
        D("stop_callbacks() done");
        D("close()");
        realuvc_->close(istream->profile_);
        D("close() done");
        istream->is_streaming_ = false;
      }
    }
    D("istream_.reset() ...");
    istream_.reset();
    D("istream_.reset() done");
    is_realuvc_ = false;
    D("realuvc_.reset()");
    realuvc_.reset();
    D("realuvc_.reset() done");
  }
  D("VideoCapture::release() done");
}

bool VideoCapture::retrieve(cv::OutputArray image, int flag) {
  if (is_opencv_) return opencv_->retrieve(image, flag);
  // FIXME: we should have grab'ed the image earlier
  return read(image);
}

bool VideoCapture::set(int prop_id, double val) {
  try {
  if (is_opencv_) return opencv_->set(prop_id, val);
  if (!is_realuvc_) return false;
  auto istream = std::dynamic_pointer_cast<VideoStream>(istream_);
  std::unique_lock<std::mutex> lock(istream->mutex_);
  int32_t ival = (int32_t)val;
  //printf("DEBUG: VideoCapture::set(%s, %.2f) ...\n", prop_name(prop_id), val); fflush(stdout);
  bool ok = false;
  switch (prop_id) {
    // properties which we can handle
    case cv::CAP_PROP_BRIGHTNESS:
      // For Leap we have a value 0..16
      return realuvc_->set_pu(RU_OPTION_BRIGHTNESS, ival);
    case cv::CAP_PROP_CONTRAST:
      return realuvc_->set_pu(RU_OPTION_CONTRAST, ival);
    case cv::CAP_PROP_FOURCC:
      istream->profile_.format = ival;
      return true;
    case cv::CAP_PROP_FPS:
      istream->profile_.fps = ival;
      break;
    case cv::CAP_PROP_FRAME_HEIGHT:
      istream->profile_.height = ival;
      return true;
    case cv::CAP_PROP_FRAME_WIDTH:
      istream->profile_.width = ival;
      return true;
    case cv::CAP_PROP_GAIN:
      ok = realuvc_->set_pu(RU_OPTION_GAIN, ival);
      //printf("DEBUG: set_pu(RU_OPTION_GAIN, %d) -> %s\n", ival, ok ? "true" : "false");
      return true;
    case cv::CAP_PROP_GAMMA:
      //printf("DEBUG: set_pu(RU_OPTION_GAMMA, %d) ...\n", ival);
      return realuvc_->set_pu(RU_OPTION_GAMMA, ival);
    case cv::CAP_PROP_SATURATION:
      return realuvc_->set_pu(RU_OPTION_SATURATION, ival);
    case cv::CAP_PROP_SHARPNESS:
      return realuvc_->set_pu(RU_OPTION_SHARPNESS, ival);
    case cv::CAP_PROP_ZOOM:
      //printf("DEBUG: set_pu(RU_OPTION_ZOOM_ABSOLUTE, %d) ...\n", ival);
      return realuvc_->set_pu(RU_OPTION_ZOOM_ABSOLUTE, ival);
    // properties we will silently ignore
    case cv::CAP_PROP_CONVERT_RGB:
    case cv::CAP_PROP_HUE:
    case cv::CAP_PROP_FORMAT:
    case cv::CAP_PROP_FRAME_COUNT:
    case cv::CAP_PROP_MODE:
    case cv::CAP_PROP_POS_AVI_RATIO:
    case cv::CAP_PROP_POS_FRAMES:
    case cv::CAP_PROP_POS_MSEC:
      return true;
    // invalid properties
    default:
      return false;
  }
  } catch (std::exception& e) {
    printf("EXCEPTION: VideoCapture::set %s\n", e.what());
    throw;
  }
  return false;
}

bool VideoCapture::is_extended() const {
  return is_realuvc_;
}

int VideoCapture::get_vendor_id() const {
  return (is_realuvc_ ? vendor_id_ : 0);
}

int VideoCapture::get_product_id() const {
  return (is_realuvc_ ? product_id_ : 0);
}

bool VideoCapture::get_xu(int ctrl, void* data, int len) {
  return false;
}

bool VideoCapture::set_xu(int ctrl, const void* data, int len) {
  return false;
}

} // end librealuvc
