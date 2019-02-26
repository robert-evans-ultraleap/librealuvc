#include <librealuvc/realuvc_driver.h>
#include <condition_variable>

namespace librealuvc {

namespace {
  
// We have a single DevMatAllocator

class DevMatAllocator : public cv::MatAllocator {
 public:
  DevMatAllocator() { }
  
  virtual ~DevMatAllocator() { }
  
  virtual cv::UMatData* allocate(
    int dims, const int* sizes, int type, void* data,
    size_t* step, cv::AccessFlag flags, cv::UMatUsageFlags usage
  ) const {
    auto alloc = cv::Mat::getDefaultAllocator();
    auto result = alloc->allocate(dims, sizes, type, data, step, flags, usage);
    if (result) result->currAllocator = alloc;
    return result;
  }
  
  virtual bool allocate(cv::UMatData* data, cv::AccessFlag flags, cv::UMatUsageFlags usage) const {
    auto alloc = cv::Mat::getDefaultAllocator();
    auto result = alloc->allocate(data, flags, usage);
    if (result) data->currAllocator = alloc;
    return result;
  }
  
  virtual void copy(
    cv::UMatData* src, cv::UMatData* dst, int dims, const size_t sz[],
    const size_t srcofs[], const size_t srcstep[],
    const size_t dstofs[], const size_t dststep[],
    bool sync
  ) const {
    auto alloc = cv::Mat::getDefaultAllocator();
    alloc->copy(
      src, dst, dims, sz, srcofs, srcstep, dstofs, dststep, sync
    );
    dst->currAllocator = alloc;
  }
  
  // deallocate() is the only method with non-default behavior
  
  virtual void deallocate(cv::UMatData* data) const {
    DevFrame* f = (DevFrame*)data->handle;
    data->handle = nullptr;
    if (f) delete f;
  }
  
  virtual void download(
    cv::UMatData* data, void* dst, int dims, const size_t sz[],
    const size_t srcofs[], const size_t srcstep[], const size_t dststep[]
  ) const {
    auto alloc = cv::Mat::getDefaultAllocator();
    alloc->download(data, dst, dims, sz, srcofs, srcstep, dststep);
  }
  
  virtual cv::BufferPoolController* getBufferPoolController(const char* id = NULL) const {
    auto alloc = cv::Mat::getDefaultAllocator();
    return alloc->getBufferPoolController();
  }
  
  virtual void map(cv::UMatData* data, cv::AccessFlag flags) const {
    auto alloc = cv::Mat::getDefaultAllocator();
    alloc->map(data, flags);
  }
  
  virtual void unmap(cv::UMatData* data) const {
    auto alloc = cv::Mat::getDefaultAllocator();
    alloc->unmap(data);
  }

  virtual void upload(
    cv::UMatData* data, const void* src, int dims, const size_t sz[],
    const size_t dstofs[], const size_t dststep[], const size_t srcstep[]
  ) {
    auto alloc = cv::Mat::getDefaultAllocator();
    alloc->upload(data, src, dims, sz, dstofs, dststep, srcstep);
  }
};

DevMatAllocator single_alloc;

} // end anon

// DevFrame methods

DevFrame::DevFrame(const frame_object& frame, const std::function<void()>& release_func) :
  cv::UMatData(&single_alloc),
  frame_(frame),
  release_func_(release_func),
  is_released_(false) {
  handle = (void*)this;
}
  
DevFrame::~DevFrame() {
  if (!is_released_) release_func_();
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
  while (size_ > 0) drop_front_locked();
}
  
void DevFrameQueue::drop_front_locked() {
  auto f = queue_[front_];
  queue_[front_] = nullptr;
  front_ = ((front_ + 1) % max_size_);
  --size_;
  delete f;
}
  
void DevFrameQueue::push_back(const frame_object& frame, const std::function<void()>& release_func) {
  std::unique_lock<std::mutex> lock(mutex_);
  while (size_ >= max_size_) drop_front_locked();
  size_t back = ((front_ + size_) % max_size_);
  queue_[back] = new DevFrame(frame, release_func);
  ++size_;
  if (num_sleepers_ > 0) {
    --num_sleepers_;
    wakeup_.notify_one();
  }
}
  
void DevFrameQueue::pop_front(cv::Mat& mat) {
  std::unique_lock<std::mutex> lock(mutex_);
  while (size_ <= 0) {
    ++num_sleepers_;
    wakeup_.wait(lock);
  }
  size_t back = ((front_ + size_) % max_size_);
  --size_;
  DevFrame* f = queue_[back];
  queue_[back] = nullptr;
  cv::UMatData* data = f;
  mat = cv::Mat(f->frame_.frame_size, 1, CV_8UC1, f->frame_.pixels);
  
  
}

} // end librealuvc
