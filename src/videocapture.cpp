// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include <librealuvc/ru_videocapture.h>
#include <librealuvc/ru_uvc.h>

namespace librealuvc {

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

double VideoCapture::get(int prop_id) {
  if (is_opencv_) return opencv_->get(prop_id);
  
}

bool VideoCapture::grab() {
  if (is_opencv_) return opencv_->grab();
  // FIXME: we should grab the image here for later retrieve()
  return true;
}

bool VideoCapture::isOpened() {
  if (is_opencv_) return opencv_->isOpened();
  return is_realcv_;
}

bool VideoCapture::open(int index) {
  
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
  read(image);
  return *this;
}

bool VideoCapture::read(cv::OutputArray image) {
  if (is_opencv_) return opencv_->read(image);
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

} // end librealuvc
