// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#ifndef LIBREALUVC_RU_VIDEOCAPTURE_H
#define LIBREALUVC_RU_VIDEOCAPTURE_H 1

// We implement a librealuvc::VideoCapture which extends the
// cv::VideoCapture, allowing code using cv::VideoCapture to
// be ported with minimal changes.

#include "ru_common.h"
#include "ru_uvc.h"
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace librealuvc {

using std::shared_ptr;
using std::unique_ptr;

// Frame data is allocated by lower-level code (e.g. Windows Media Foundation),
// so cv::Mat's referring to lower-level buffers will have a custom
// cv::MatAllocator to release the buffer when it's no longer being used
// by any cv::Mat.

// librealuvc::VideoCapture overrides all methods of cv::VideoCapture,
// but it may represent either a librealuvc device or an opencv device.
// When it's an opencv device, it will forward method calls to get
// the normal cv::VideoCapture behavior.

class IVideoStream {
 public:
  virtual ~IVideoStream() { }
};

class LIBREALUVC_EXPORT VideoCapture : public cv::VideoCapture {
 protected:
  bool is_opencv_;
  unique_ptr<cv::VideoCapture> opencv_;
  bool is_realuvc_;
  shared_ptr<librealuvc::uvc_device> realuvc_;
  shared_ptr<IVideoStream> istream_;
  
 public:
  VideoCapture();
  VideoCapture(int index);
  VideoCapture(const cv::String& filename);
  VideoCapture(const cv::String& filename, int api_reference);
  
  virtual ~VideoCapture();
  
  virtual double get(int propId) const;
  virtual bool grab();
  virtual bool isOpened() const;
  
  virtual bool open(int index);
  virtual bool open(const cv::String& filename);
  virtual bool open(const cv::String& filename, int api_preference);
  
  virtual VideoCapture& operator>>(cv::Mat& image);
  virtual VideoCapture& operator>>(cv::UMat& image);
  
  virtual bool read(cv::OutputArray image);
  virtual void release();
  virtual bool retrieve(cv::OutputArray image, int flag = 0);
  virtual bool set(int propId, double value);
};
  
} // end librealuvc


#endif
