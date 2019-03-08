
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

int nframe;

int do_stuff() {
  bool ok;
  librealuvc::VideoCapture cap;
  int camera_id = 1;
  ok = cap.open(camera_id);
  D("cap.open(%d) -> %s", camera_id, ok ? "true" : "false");
  if (!cap.is_extended()) {
    D("camera_id %d is an opencv device\n"); 
  } else {
    int vendor_id = cap.get_vendor_id();
    int product_id = cap.get_product_id();
    D("camera_id %d has vendor_id 0x%04x product_id 0x%04x\n",
      camera_id, vendor_id, product_id);
  }
  if (0) {
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS, 45.0);
  } else {
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 384);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 384);
    cap.set(cv::CAP_PROP_FPS, 90.0);
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
