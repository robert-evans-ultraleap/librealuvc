#include "types.h"
#include "backend.h"
#include <chrono>
#include <cstdio>
#include <exception>
#include <mutex>
#include <string>
#include <thread>

#if 0
#define D(...) { }
#else
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#endif

using namespace librealsense;

void notifications_processor::raise_notification(const notification n) {
  printf("WARNING: raise_notification()\n");
  fflush(stdout);
}

int nframe;

void on_frame(
  platform::stream_profile prof,
  platform::frame_object frame,
  std::function<void()> func
) {
  ++nframe;
  if (1) {
    printf("DEBUG: nframe %d ...\n", nframe);
    fflush(stdout);
  }
  func();
}

uint32_t str2fourcc(const char* s) {
  uint32_t result = 0;
  for (int j = 0; j < 4; ++j) {
    uint32_t b = ((int)s[j] & 0xff);
    result |= (b << 8*(3-j));
  }
  printf("DEBUG: str2fourcc(\"%s\") -> 0x%08lx\n", s, (long)result);
  return result;
}

int do_stuff() {
  D("create_backend() ...");
  fflush(stdout);
  auto backend = librealsense::platform::create_backend();
  D("backend %p", backend.get());
  auto uvc_info = backend->query_uvc_devices();
  D("query_uvc_devices() size %d", (int)uvc_info.size());
  for (size_t idx = 0; idx < uvc_info.size(); ++idx) {
    auto& info = uvc_info[idx];
    printf("uvc[%d]:\n", (int)idx);
    printf("  vid: 0x%04x\n", (int)info.vid);
    printf("  pid: 0x%04x\n", (int)info.pid);
    auto dev = backend->create_uvc_device(info);
    D("create_uvc_device() -> %s", dev ? "pass" : "fail");
    dev->set_power_state(platform::D0); // device fully on
    platform::stream_profile prof;
    prof.width = 384;
    prof.height = 384;
    prof.fps = 90;
    prof.format = str2fourcc("YUY2");
    D("probe_and_commit() ...");
    dev->probe_and_commit(prof, on_frame);
    D("stream_on() ...");
    dev->stream_on();
    D("start_callbacks() ...");
    dev->start_callbacks();
    for (int j = 0; j < 10; ++j) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      D("main thread wakeup");
    }
  }
  D("exit ...");
  return 0;
}

extern "C"
int main() {
  try {
    do_stuff();
  } catch (std::exception e) {
	  printf("ERROR: caught exception %s\n", e.what());
  }
}
