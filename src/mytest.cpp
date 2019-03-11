
#include <librealuvc/ru_videocapture.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <cstdio>

#if 0
#define D(...) { }
#else
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#endif

using namespace librealuvc;

uint32_t str2fourcc(const char* s) {
  uint32_t result = 0;
  for (int j = 0; j < 4; ++j) {
    uint32_t b = ((int)s[j] & 0xff);
    result |= (b << 8*(3-j));
  }
  return result;
}

int nframe;

#define VENDOR_LEAPMOTION 0x2936
#define VENDOR_MSI        0x5986
#define VENDOR_LOGITECH   0x046d

#define PRODUCT_GS63CAM   0x0547
#define PRODUCT_C905      0x080a

int do_stuff() {
  bool ok;
  librealuvc::VideoCapture cap;
  for (int id = 0; id < 2; ++id) {
    ok = cap.open(id);
    if (ok) {
      D("camera_id %d vendor 0x%04x product 0x%04x", 
        id, cap.get_vendor_id(), cap.get_product_id());
    }
  }
  int camera_id = 1;
  ok = cap.open(camera_id);
  D("cap.open(%d) -> %s", camera_id, ok ? "true" : "false");
  int vendor_id = 0;
  int product_id = 0;
  if (!cap.is_extended()) {
    D("camera_id %d is an opencv device\n", camera_id);
  } else {
    vendor_id = cap.get_vendor_id();
    product_id = cap.get_product_id();
    D("camera_id %d has vendor_id 0x%04x product_id 0x%04x\n",
      camera_id, vendor_id, product_id);
  }
  switch (vendor_id) {
    case VENDOR_LEAPMOTION:
      cap.set(cv::CAP_PROP_FOURCC, str2fourcc("YUY2"));
      cap.set(cv::CAP_PROP_FRAME_WIDTH, 384);
      cap.set(cv::CAP_PROP_FRAME_HEIGHT, 384);
      cap.set(cv::CAP_PROP_FPS, 90.0);
      break;
    case VENDOR_LOGITECH:
      cap.set(cv::CAP_PROP_FOURCC, str2fourcc("I420"));
      cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
      cap.set(cv::CAP_PROP_FRAME_HEIGHT, 400);
      cap.set(cv::CAP_PROP_FPS, 15.0);
      break;
    case VENDOR_MSI:
      cap.set(cv::CAP_PROP_FOURCC, str2fourcc("NV12"));
      cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
      cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
      cap.set(cv::CAP_PROP_FPS, 30.0);
      break;
    default:
      printf("ERROR: unknown vendor 0x%04x\n", vendor_id);
      exit(1);
  }
  cv::Mat mat;
  while (1) {
    ok = cap.read(mat);
    if (!ok) {
      D("cap.read() fail\n");
      break;
    }
    cv::imshow("Camera_LR", mat);
    int c = (int)cv::waitKey(1);
    if ((c == 27) || (c == 'q')) break; // <ESC> or 'q' key
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
  cv::destroyAllWindows();
  return 0;
}
