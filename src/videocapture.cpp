// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include <librealuvc/ru_videocapture.h>
#include <librealuvc/ru_uvc.h>
#include <librealuvc/realuvc.h>
#include <opencv2/core/mat.hpp>

namespace librealuvc {

namespace {

// Support for managing buffer lifetime without copying
//
// The cv::Mat's populated

class DevMatAllocator : public cv::MatAllocator {
 private:
  cv::MatAllocator* default_alloc_;
  
 public:
  inline DevMatAllocator() :
    default_alloc_(cv::Mat::getDefaultAllocator()) {
  }
  
  virtual ~DevMatAllocator() { }
  
  virtual cv::UMatData* allocate(int dims, const int* sizes, int type, void* data,
    size_t* step, cv::AccessFlag flags, cv::UMatUsageFlags usage) const {
    return default_alloc_->allocate(dims, sizes, type, data, step, flags, usage);
  }
  
  virtual bool allocate(cv::UMatData* data, cv::AccessFlag access, cv::UMatUsageFlags usage) const {
    return default_alloc_->allocate(data, access, usage);
  }

  virtual void deallocate(cv::UMatData* data) const {
    //
  }
};

DevMatAllocator* get_single_allocator() {
  static DevMatAllocator single;
  return &single;
}

// A realuvc-managed device will pass frame buffers by callback. These will
// wrapped in a cv::Mat and queued until a VideoCapture::read(), VideoCapture::retrieve(),
// or being dropped due to queue overflow.

} // end anon

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
  // FIXME
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
  // FIXME
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
  // FIXME
  return false;
}

} // end librealuvc
