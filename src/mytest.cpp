#include "types.h"
#include "backend.h"
#include "leap_xu.h"
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

using namespace librealuvc;
using platform::uvc_device;

void notifications_processor::raise_notification(const notification n) {
  printf("WARNING: raise_notification()\n");
  fflush(stdout);
}

int nframe;

void on_frame(
  platform::stream_profile prof,
  platform::frame_object frame,
  std::function<void()> release_frame
) {
  ++nframe;
  if (1) {
    printf("DEBUG: nframe %d ...\n", nframe);
    fflush(stdout);
  }
  release_frame();
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

std::vector<platform::extension_unit> try_all_xus() {
  std::vector<platform::extension_unit> result;
  platform::guid id = LEAP_XU_GUID;
  for (int sub = 0; sub <= 0; ++sub) {
    for (int unit = 1; unit <= 1; ++unit) {
      for (int node = 4; node <= 4; ++node) {
        platform::extension_unit xu;
        xu.subdevice = sub;
        xu.unit = unit;
        xu.node = node;
        xu.id = id;
        result.push_back(xu);
      }
    }
  }
  return result;
}

platform::extension_unit leap_xu() {
  platform::extension_unit xu;
  xu.subdevice = 0;
  xu.unit = 1;
  xu.node = 0;
  platform::guid id = LEAP_XU_GUID;
  xu.id = id;
  return xu;
}

void init_leap_xu(std::shared_ptr<uvc_device> dev) {
  auto all = try_all_xus();
  for (auto& xu : all) {
    D("init_xu() ... { subdev %d, unit %d, node %d }",
      (int)xu.subdevice, (int)xu.unit, (int)xu.node);
    dev->init_xu(xu);
    D("init_xu() done");
  }
}

int get_leap_xu(std::shared_ptr<uvc_device> dev, int ctrl, void* buf, int len) {
  auto all = try_all_xus();
  for (auto& xu : all) {
    D("get_xu(ctrl %d) ... { subdev %d, unit %d, node %d }",
      (int)ctrl, (int)xu.subdevice, (int)xu.unit, (int)xu.node);
    bool ok = false;
    try {
      bool ok = dev->get_xu(xu, ctrl, (uint8_t*)buf, len);
      D("get_xu() PASS");
      return 0;
    } catch (std::exception e) {
      D("ERROR: caught exception %s", e.what());
    }
    D("get_xu() done");
    if (!ok) {
      printf("ERROR: get_leap_xu(ctrl %d) fail", ctrl);
    }
  }
  return 0;
}

void show_leap_xus(std::shared_ptr<uvc_device>) {
  
}

void show_leap_devcaps(std::shared_ptr<uvc_device> dev) {
  char buf[0x100];
  memset(buf, 0, 0x100);
  LEAP_DEVCAPS* r = (LEAP_DEVCAPS*)&buf[0];
  get_leap_xu(dev, LEAP_XU_DEVCAPS, r, sizeof(*r));
  printf("LEAP_DEVCAPS:\n");
  printf("  flags 0x%08x\n", (int)r->flags);
  printf("  firmware_rev 0x%08x\n", (int)r->firmware_rev);
  printf("  controller_id %d\n", (int)r->controller_id);
  printf("  sensor_id %d\n", (int)r->sensor_id);
  printf("  serial %s\n", r->serial);
}

int do_stuff() {
  D("create_backend() ...");
  fflush(stdout);
  auto backend = librealuvc::platform::create_backend();
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
    if (info.vid == 0x2936) {
      init_leap_xu(dev);
      show_leap_devcaps(dev);
    }
    
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
    for (int j = 0; j < 2; ++j) {
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
