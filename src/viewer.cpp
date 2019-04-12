
#include <librealuvc/ru_videocapture.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdio>
#include <cstrings>

#if 1
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define D(...) { }
#endif

using namespace librealuvc;
using std::string;

class OptionParse {
 private:
  int    argc_;
  char** argv_;
  int    idx_;
  char*  pos_;

 public:
  OptionParse(int argc, char** argv) :
    argc_(argc),
    argv_(argv),
    idx_(1),
    pos_(nullptr) {
  }
  
  bool advance() {
    if (pos_ && (pos_ == argv_[idx_])) ++idx_;
    pos_ = nullptr;
    return true;
  }

  bool have_option(const char* short_name, const char* long_name) {
    if (idx_ >= argc_) return false;
    auto a = argv_[idx_];
    auto len = strlen(a);
    if ((len < 2) || (a[0] != '-')) return false;
    if (a[1] == short_name[1]) {
      if (len > 2) {
        pos_ = &a[2];
      } else {
        ++idx_;
        pos_ = ((idx_ < argc_) ? argv_[idx_] : nullptr);
      }
      return advance();
    } else if (!strcmp(a, long_name)) {
      ++idx_;
      pos_ = ((idx_ < argc_) ? argv_[idx_] : nullptr);
      return advance();
    }
    return false;
  }
  
  bool have_bool(bool* val) {
    if (!pos_) return false;
    if (!strcasecmp(val, "false") ||
        !strcasecmp(val, "off") ||
        !strcasecmp(val, "no")) {
      *val = false;
      return advance();
    }
    if (!strcasecmp(val, "true") ||
        !strcasecmp(val, "on") ||
        !strcasecmp(val, "yes")) {
      *val = true;
      return advance();
    }
    return false;
  }
  
  bool have_double(double* val) {
    if (!pos_) return false;
  }
  
  bool have_int(int* val) {
    if (!pos_) return false;
  }
  
  bool have_string(string* val) {
    if (!pos_) return false;
    *val = std::string(pos_);
    return advance();
  }
  
  bool have_end() {
    return (idx_ >= argc_);
  }
};

class ViewerOptions {
 public:
  
 
 public:
  ViewerOptions(int argc, char** argv) {
    OptionParse p(argc, argv);
    while (!p.have_end()) {
      if (have_option("
    }
  }

  bool is_match
};

ViewerOptions::ViewerOptions(int argc, char* argv[]) {
}

uint32_t str2fourcc(const char* s) {
  uint32_t result = 0;
  for (int j = 0; j < 4; ++j) {
    uint32_t b = ((int)s[j] & 0xff);
    result |= (b << 8*(3-j));
  }
  return result;
}

int nframe;

#define VENDOR_LEAPMOTION  0x2936
#define VENDOR_MSI         0x5986
#define VENDOR_LOGITECH    0x046d
#define VENDOR_PERIPHERAL  0xf182

#define PRODUCT_GS63CAM    0x0547
#define PRODUCT_C905       0x080a
#define PRODUCT_PERIPHERAL 0x0003

void open_camera(librealuvc::VideoCapture& camera, ViewerOptions& opt) {
}

void view_camera(librealuvc::VideoCapture& camera, ViewerOptions& opt) {
}

int view_camera() {
  bool ok;
  int camera_id = -1;
  librealuvc::VideoCapture cap;
  for (int id = 0; id < 2; ++id) {
    ok = cap.open(id);
    if (ok) {
      D("camera_id %d vendor 0x%04x product 0x%04x", 
        id, cap.get_vendor_id(), cap.get_product_id());
      camera_id = id;
      if ((cap.get_vendor_id() == VENDOR_LEAPMOTION) ||
          (cap.get_vendor_id() == VENDOR_PERIPHERAL)) {
        break;
      }
    }
  }
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
    case VENDOR_PERIPHERAL:
      cap.set(cv::CAP_PROP_FOURCC, str2fourcc("YUY2"));
      cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
      cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
      cap.set(cv::CAP_PROP_FPS, 57.5);
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
  for (int count = 0;; ++count) {
    ok = cap.read(mat);
    if (!ok) {
      D("cap.read() fail\n");
      break;
    }
    cv::imshow("Camera_LR", mat);
    if ((count % 100) == 0) {
      show_blowup(mat);
    }
    int c = (int)cv::waitKey(1);
    if ((c == 27) || (c == 'q')) break; // <ESC> or 'q' key
  }
  D("exit ...");
  return 0;
}

extern "C"
int main(int argc, char* argv[]) {
  ViewerOptions options(argc, argv);
  try {
    librealuvc::VideoCapture camera;
    open_camera(camera, options);
    if (camera.isOpened()) {
      view_camera(camera, options);
    }
  } catch (std::exception e) {
	  fprintf(stderr, "ERROR: caught exception %s\n", e.what());
  }
  cv::destroyAllWindows();
  return 0;
}
