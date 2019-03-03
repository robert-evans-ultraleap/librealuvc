#include <librealuvc/realuvc_driver.h>
#include <condition_variable>

#if 0
#define D(...) { }
#else
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#endif

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
    D("deallocate(umatdata %p) DevFrame %p", data, data->handle);
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
    // From reading the source code of OpenCV, it turns out that unmap()
    // is the method called when the refcount goes to zero.
    D("unmap(umatdata %p) DevFrame %p", data, data->handle);
    DevFrame* f = (DevFrame*)data->handle;
    data->handle = nullptr;
    if (f) delete f;
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

DevFrame::DevFrame(
  const stream_profile& profile,
  const frame_object& frame,
  const std::function<void()>& release_func
) :
  cv::UMatData(&single_alloc),
  profile_(profile),
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
  
void DevFrameQueue::push_back(
  const stream_profile& profile,
  const frame_object& frame,
  const std::function<void()>& release_func
) {
  std::unique_lock<std::mutex> lock(mutex_);
  while (size_ >= max_size_) drop_front_locked();
  size_t back = ((front_ + size_) % max_size_);
  queue_[back] = new DevFrame(profile, frame, release_func);
  ++size_;
  if (num_sleepers_ > 0) {
    --num_sleepers_;
    wakeup_.notify_one();
  }
}

void print_mat(const char* what, const cv::Mat& mat) {
  printf("print_mat(%s):\n", what);
  printf("  allocator %p (DevMatAllocator %p)\n", mat.allocator, &single_alloc);
  printf("  cols %d\n", mat.cols);
  printf("  data %p\n", mat.data);
  printf("  dims %d\n", mat.dims);
  printf("  flags 0x%08x\n", mat.flags);
  printf("  rows %d\n", mat.rows);
  for (int j = 0; j < mat.size.dims(); ++j) {
    printf("  size[%d] %ld\n", j, (long)mat.size[j]);
  }
  printf("  step %ld\n", (long)mat.step);
  printf("  umatdata %p\n", mat.u);
  fflush(stdout);
  auto u = mat.u;
  if (!u) return;
  printf("    allocatorFlags_ 0x%08x\n", u->allocatorFlags_);
  printf("    currAllocator %p\n", u->currAllocator);
  printf("    data %p\n", u->data);
  printf("    flags 0x%08x\n", (int)u->flags);
  printf("    handle %p\n", u->handle);
  printf("    mapcount %d\n", u->mapcount);
  printf("    origdata %p\n", u->origdata);
  printf("    origUMatData %p\n", u->originalUMatData);
  printf("    prevAllocator %p\n", u->prevAllocator);
  printf("    refcount %d\n", u->refcount);
  printf("    size %ld\n", (long)u->size);
  printf("    urefcount %d\n", u->urefcount);
  printf("    userdata %p\n", u->userdata);
  printf("\n");
  fflush(stdout);
}
  
void DevFrameQueue::pop_front(cv::Mat& mat) {
  std::unique_lock<std::mutex> lock(mutex_);
  while (size_ <= 0) {
    ++num_sleepers_;
    wakeup_.wait(lock);
  }
  size_t front = front_;
  DevFrame* f = queue_[front];
  queue_[front] = nullptr;
  front_ = ((front + 1) % max_size_);
  --size_;
  cv::UMatData* data = f;
  D("pop_front DevFrame %p", f);
  cv::Mat m;
  D("A");
  m.allocator = &single_alloc;
  m.cols = f->profile_.height;
  m.data = (uchar*)f->frame_.pixels;
  m.dims = 2;
  // m.flags ?
  m.rows = f->profile_.width;
  m.step = m.cols;
  D("B");
  m.u = data;
  data->data = m.data;
  data->refcount = 1;
  data->size = 1;
  D("C");
  print_mat("m", m);
  D("D");
  mat = m;
  D("E");
}

} // end librealuvc
