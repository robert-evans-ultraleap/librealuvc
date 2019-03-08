// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include <librealuvc/ru_videocapture.h>
#include <librealuvc/ru_uvc.h>
#include <librealuvc/realuvc.h>
#include <opencv2/core/mat.hpp>
#include <librealuvc/realuvc_driver.h>

#if 0
#define D(...) { }
#else
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#endif

namespace librealuvc {

namespace {

uint32_t str2fourcc(const char* s) {
  uint32_t result = 0;
  for (int j = 0; j < 4; ++j) {
    uint32_t b = ((int)s[j] & 0xff);
    result |= (b << 8*(3-j));
  }
  D("str2fourcc(\"%s\") -> 0x%x", s, result);
  return result;
}

// A realuvc-managed device will pass frame buffers by callback. These will
// wrapped in a cv::Mat and queued until a VideoCapture::read(), 
// VideoCapture::retrieve(), or being dropped due to queue overflow.

} // end anon

class VideoStream : public IVideoStream {
 public:
  stream_profile profile_;
  bool is_streaming_;
  DevFrameQueue queue_;
  
 public:
  VideoStream(int max_size = 1) :
    is_streaming_(false),
    queue_(max_size) {
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
}

double VideoCapture::get(int prop_id) const {
  if (is_opencv_) return opencv_->get(prop_id);
  if (!is_realuvc_) return 0.0;
  auto istream = std::dynamic_pointer_cast<VideoStream>(istream_);
  switch (prop_id) {
    // properties which we can handle
    case cv::CAP_PROP_FRAME_WIDTH:
      return (double)istream->profile_.width;
    case cv::CAP_PROP_FRAME_HEIGHT:
      return (double)istream->profile_.height;
    case cv::CAP_PROP_FPS:
      return (double)istream->profile_.fps;
      break;
    case cv::CAP_PROP_BRIGHTNESS:
      break;
    case cv::CAP_PROP_SATURATION:
      break;
    case cv::CAP_PROP_GAIN:
      break;
    case cv::CAP_PROP_CONVERT_RGB:
      break;
    case cv::CAP_PROP_FOURCC:
      return (double)istream->profile_.format;
      break;
    // properties we will silently ignore
    case cv::CAP_PROP_POS_MSEC:
    case cv::CAP_PROP_POS_FRAMES:
    case cv::CAP_PROP_POS_AVI_RATIO:
    case cv::CAP_PROP_FRAME_COUNT:
    case cv::CAP_PROP_FORMAT:
    case cv::CAP_PROP_MODE:
    case cv::CAP_PROP_CONTRAST:
    case cv::CAP_PROP_HUE:
      return 0.0;
    // invalid properties
    default:
      return 0.0;
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
  istream_ = std::make_shared<VideoStream>();
  realuvc_->set_power_state(D0);
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
  if (is_opencv_) return opencv_->read(image);
  if (!is_realuvc_) return false;
  auto istream = std::dynamic_pointer_cast<VideoStream>(istream_);
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
      D("stream_on() done");
      D("start_callbacks() ...");
      realuvc_->start_callbacks();
      D("start_callbacks() done");
      istream->is_streaming_ = true;
    } catch (std::exception e) {
      printf("ERROR: caught exception %s\n", e.what());
      fflush(stdout);      
    }
  }
  if (!istream->is_streaming_) return false;
  cv::Mat tmp;
  istream->queue_.pop_front(tmp); // wait for a frame if necessary
  if (image.needed()) {
    // OutputArray::assign() will not copy unless it needs to
    image.assign(tmp);
  }
  return true;
}

void VideoCapture::release() {
  if (is_opencv_) opencv_->release();
  if (is_realuvc_) {
    is_realuvc_ = false;
    realuvc_.reset();
  }
}

bool VideoCapture::retrieve(cv::OutputArray image, int flag) {
  if (is_opencv_) return opencv_->retrieve(image, flag);
  // FIXME: we should have grab'ed the image earlier
  return read(image);
}

bool VideoCapture::set(int prop_id, double val) {
  if (is_opencv_) return opencv_->set(prop_id, val);
  if (!is_realuvc_) return false;
  auto istream = std::dynamic_pointer_cast<VideoStream>(istream_);
  switch (prop_id) {
    // properties which we can handle
    case cv::CAP_PROP_FRAME_WIDTH:
      istream->profile_.width = (int)val;
      return true;
    case cv::CAP_PROP_FRAME_HEIGHT:
      istream->profile_.height = (int)val;
      return true;
    case cv::CAP_PROP_FPS:
      istream->profile_.fps = (int)val;
      break;
    case cv::CAP_PROP_BRIGHTNESS:
      break;
    case cv::CAP_PROP_SATURATION:
      break;
    case cv::CAP_PROP_GAIN:
      break;
    case cv::CAP_PROP_CONVERT_RGB:
      break;
    // properties we will silently ignore
    case cv::CAP_PROP_POS_MSEC:
    case cv::CAP_PROP_POS_FRAMES:
    case cv::CAP_PROP_POS_AVI_RATIO:
    case cv::CAP_PROP_FOURCC:
    case cv::CAP_PROP_FRAME_COUNT:
    case cv::CAP_PROP_FORMAT:
    case cv::CAP_PROP_MODE:
    case cv::CAP_PROP_CONTRAST:
    case cv::CAP_PROP_HUE:
      return true;
    // invalid properties
    default:
      return false;
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
