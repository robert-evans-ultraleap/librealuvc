#include "types.h"
#include "backend.h"
#include <cstdio>

using namespace librealsense;

void notifications_processor::raise_notification(const notification n) {
}

extern "C"
int main() {
  auto backend = librealsense::platform::create_backend();
  auto uvc_info = backend->query_uvc_devices();
  for (size_t idx = 0; idx < uvc_info.size(); ++idx) {
    auto& info = uvc_info[idx];
    printf("uvc[%d]:\n", (int)idx);
    printf("  vid: 0x%04x\n", (int)info.vid);
    printf("  pid: 0x%04x\n", (int)info.pid);
    auto dev = backend->create_uvc_device(info);
    printf("DEBUG: create_uvc_device() -> %s\n", dev ? "pass" : "fail");
  }
  return 0;
}
