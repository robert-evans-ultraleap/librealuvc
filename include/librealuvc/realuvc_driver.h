#ifndef REALUVC_REALUVC_DRIVER_H
#define REALUVC_REALUVC_DRIVER_H 1
//
// Support for loadable camera-device drivers
//
// Copyright (c) Leap Motion Corp 2019.  All rights reserved.
//
#include <librealuvc/realuvc.h>
#include <opencv2/core.hpp>

namespace librealuvc {

using std::shared_ptr;
using std::vector;

// Support for managing buffer lifetime without copying
//
// The cv::Mat's populated by librealuvc may refer to buffers
// from a lower-level video framework. 

class DevFrame : public cv::MatAllocator {
 private:
  bool is_released_;
  frame_object frame_;
  std::function<void()> release_func_;
 
 public:
  DevFrame(const frame_object& frame, const std::function<void()>& release_func);
  
  virtual ~DevFrame();
  
  virtual cv::UMatData* allocate(
    int dims, const int* sizes, int type, void* data,
    size_t* step, cv::AccessFlag flags, cv::UMatUsageFlags usage
  ) const;
  
  virtual bool allocate(cv::UMatData* data, cv::AccessFlag flags, cv::UMatUsageFlags usage) const;
  
  virtual void copy(
    cv::UMatData* src, cv::UMatData* dst, int dims, const size_t sz[],
    const size_t srcofs[], const size_t srcstep[],
    const size_t dstofs[], const size_t dststep[],
    bool sync
  ) const;
  
  virtual void deallocate(cv::UMatData* data) const;
  
  virtual void download(
    cv::UMatData* data, void* dst, int dims, const size_t sz[],
    const size_t srcofs[], const size_t srcstep[], const size_t dststep[]
  ) const;
  
  virtual cv::BufferPoolController* getBufferPoolController(const char* id = NULL) const;
  
  virtual void map(cv::UMatData* data, cv::AccessFlag flags) const;
  
  virtual void unmap(cv::UMatData* data) const;
   
  virtual void upload(
    cv::UMatData* data, const void* src, int dime, const size_t sz[],
    const size_t dstofs[], const size_t dststep[], const size_t srcstep[]
  );  
};

class DevFrameQueue {
 private:
  std::mutex mutex_;
  std::condition_variable wakeup_;
  int num_sleepers_;
  size_t max_size_;
  size_t size_;
  size_t front_;
  vector< shared_ptr<DevFrame> > queue_;
 
 public:
  DevFrameQueue(size_t max_size = 1);
  
  ~DevFrameQueue();
  
  void drop_front_locked();
  
  void push_back(const frame_object& frame, const std::function<void()>& release_func);
  
  shared_ptr<DevFrame> pop_front();  
};

} // end librealuvc

#endif