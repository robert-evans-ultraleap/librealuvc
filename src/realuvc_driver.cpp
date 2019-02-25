#include <librealuvc/realuvc_driver.h>
#include <condition_variable>

namespace librealuvc {

// DevFrame methods

DevFrame::DevFrame(const frame_object& frame, const std::function<void()>& release_func) :
  is_released_(false),
  frame_(frame),
  release_func_(release_func) {
}
  
DevFrame::~DevFrame() {
  if (!is_released_) {
    is_released_ = true;
    release_func_();
  }
}
  
cv::UMatData* DevFrame::allocate(
  int dims, const int* sizes, int type, void* data,
  size_t* step, cv::AccessFlag flags, cv::UMatUsageFlags usage
) const {
  assert(0);
}
  
bool DevFrame::allocate(cv::UMatData* data, cv::AccessFlag flags, cv::UMatUsageFlags usage) const {
  assert(0);
  return false;
}
  
void DevFrame::copy(
  cv::UMatData* src, cv::UMatData* dst, int dims, const size_t sz[],
  const size_t srcofs[], const size_t srcstep[],
  const size_t dstofs[], const size_t dststep[],
  bool sync
) const {
  cv::Mat::getDefaultAllocator()->copy(
    src, dst, dims, sz, srcofs, srcstep, dstofs, dststep, sync
  );
}
  
void DevFrame::deallocate(cv::UMatData* /*data*/) const {
  DevFrame* mutable_this = const_cast<DevFrame*>(this);
  mutable_this->is_released_ = true;
  release_func_();
  // When this is called, there's no more reference to this DevFrame,
  // so we should delete it.
  delete mutable_this;
}

void DevFrame::download(
  cv::UMatData* data, void* dst, int dims, const size_t sz[],
  const size_t srcofs[], const size_t srcstep[], const size_t dststep[]
) const {
  cv::Mat::getDefaultAllocator()->download(
    data, dst, dims, sz, srcofs, srcstep, dststep
  );
}
  
cv::BufferPoolController* DevFrame::getBufferPoolController(const char* id) const {
  return cv::Mat::getDefaultAllocator()->getBufferPoolController(id);
}
  
void DevFrame::map(cv::UMatData* data, cv::AccessFlag flags) const {
  cv::Mat::getDefaultAllocator()->map(data, flags);
}
  
void DevFrame::unmap(cv::UMatData* data) const {
  cv::Mat::getDefaultAllocator()->unmap(data);
}
   
void DevFrame::upload(
  cv::UMatData* data, const void* src, int dims, const size_t sz[],
  const size_t dstofs[], const size_t dststep[], const size_t srcstep[]
) {
  cv::Mat::getDefaultAllocator()->upload(
    data, src, dims, sz, dstofs, dststep, srcstep
  );
}    
  
// DevFrameQueue methods

DevFrameQueue::DevFrameQueue(size_t max_size) {
  num_sleepers_ = 0;
  if (max_size < 1) max_size = 1;
  max_size_ = max_size;
  size_ = 0;
  front_ = 0;
  queue_.resize(max_size_);
}
  
DevFrameQueue::~DevFrameQueue() {
}
  
void DevFrameQueue::drop_front_locked() {
  queue_[front_].reset();
  front_ = ((front_ + 1) % max_size_);
  --size_;
}
  
void DevFrameQueue::push_back(const frame_object& frame, const std::function<void()>& release_func) {
  std::unique_lock<std::mutex> lock(mutex_);
  while (size_ >= max_size_) drop_front_locked();
  size_t back = ((front_ + size_) % max_size_);
  queue_[back] = std::make_shared<DevFrame>(frame, release_func);
  ++size_;
  if (num_sleepers_ > 0) {
    --num_sleepers_;
    wakeup_.notify_one();
  }
}
  
shared_ptr<DevFrame> DevFrameQueue::pop_front() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (size_ <= 0) {
    ++num_sleepers_;
    wakeup_.wait(lock);
  }
  size_t back = ((front_ + size_) % max_size_);
  --size_;
  return std::move(queue_[back]);
}

} // end librealuvc
