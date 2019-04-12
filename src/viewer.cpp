
#include <librealuvc/ru_videocapture.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstrings>
#include <optional>

#if 1
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define D(...) { }
#endif

using namespace librealuvc;
using std::string;

class OptionParse {
 public:
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
    char* endp = pos_;
    *val = strtod(pos_, &endp);
    return ((endp > pos_) && (*endp == 0) && advance());
  }
  
  bool have_int(int* val) {
    if (!pos_) return false;
    char* endp = pos_;
    *val = (int)strtol(pos_, &endp, 0);
    return ((endp > pos_) && (*endp == 0) && advance());
  }
  
  bool have_string(string* val) {
    if (!pos_) return false;
    *val = std::string(pos_);
    return ((val->length() > 0) && advance());
  }
  
  bool have_end() {
    return (idx_ >= argc_);
  }
};

template<typename T>

class ViewerOptions {
 public:
  std::optional<string> product_;
  std::optional<double> fps_;
  std::optional<int> height_;
  std::optional<int> width_;
 
 public:
  void usage() {
    fprintf(
      stderr,
      "usage: viewer [--fps <num>] [--height <num>] [--width <num>] [--product <name>]\n"
      "  --fps <num>         frames-per-second\n"
      "  --height <num>      frame height in pixels\n"
      "  --width <num>       frame width in pixels\n"
      "  --product <string>  choose rigel or leap device\n"
    );
    exit(1);
  }

  ViewerOptions(int argc, char** argv) {
    OptionParse p(argc, argv);
    double dval;
    int ival;
    string sval;
    while (!p.have_end()) {
      if        (p.have_option("-?", "--help")) {
        usage();
      } else if (p.have_option("-f", "--fps")) {
        if (!p.have_double(&dval)) usage();
        fps_ = dval;
      } else if (p.have_option("-h", "--height")) {
        if (!p.have_int(&ival)) usage();
        height_ = ival;
      } else if (p.have_option("-p", "--product")) {
        if (!p.have_string(&sval)) usage();
        product_ = sval;
      } else if (p.have_option("-w", "--width")) {
        if (!p.have_int(&ival)) usage();
        width_ = ival;
      } else {
        fprintf(stderr, "ERROR: unexpected command-line option '%s'\n", p.argv_[p.idx_]);
        usage();
      }
    }
  }

  bool is_match(
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

#define VENDOR_RIGEL       0x2936
#define VENDOR_MSI         0x5986
#define VENDOR_LOGITECH    0x046d
#define VENDOR_LEAP        0xf182

#define PRODUCT_RIGEL      0x1202
#define PRODUCT_GS63CAM    0x0547
#define PRODUCT_C905       0x080a
#define PRODUCT_LEAP       0x0003

bool is_rigel(librealuvc::VideoCapture& cap) {
  return (cap.isOpened() && 
    (cap.get_vendor_id() == VENDOR_RIGEL) && 
    (cap.get_product_id() == PRODUCT_RIGEL));
}

bool is_leap(librealuvc::VideoCapture& cap) {
  return (cap.isOpened() && 
    (cap.get_vendor_id() == VENDOR_LEAP) && 
    (cap.get_product_id() == PRODUCT_LEAP));
}

struct Config {
  double fps_;
  int width_;
  int height_;
};

Config config_leap[] = {
  {  24.0, 800, 800 },
  {  45.0, 640, 480 },
  {  70.0, 680, 286 },
  {  85.0, 400, 400 },
  {  90.0, 384, 384 },
  { 100.0, 368, 368 },
  { 0, 0, 0 }
};

Config config_rigel[] = {
  {  50.0, 752, 480 },
  {  57.5, 640, 480 },
  { 100.0, 752, 240 },
  { 115.0, 640, 240 },
  { 190.0, 752, 120 },
  { 214.0, 640, 120 },
  { 0, 0, 0}
};

Config config_default[] = {
  {  30.0, 640, 480 },
  { 0, 0, 0 }
};

void config_cap(librealuvc::VideoCapture& cap, ViewerOptions& opt) {
  Config* table = config_default;
  if      (is_leap(cap))  table = config_leap;
  else if (is_rigel(cap)) table = config_rigel;
  for (Config* p = table; p->width_ > 0; ++p) {
    if (opt.fps_.has_value()    && (opt.fps_ != p->fps_)) continue;
    if (opt.width_.has_value()  && (opt.width_ != p->width_)) continue;
    if (opt.height_.has_value() && (opt.height_ != p->height_)) continue;
    // Everything matched
    cap.set(cv::CAP_PROP_FOURCC, str2fourcc("YUY2"));
    cap.set(cv::CAP_PROP_FPS,          p->fps_);
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  p->width_);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, p->height_);
    return;
  }
  fprintf(stderr, "ERROR: invalid fps, width, height\n");
  cap.release();
  exit(1);
}

void open_cap(librealuvc::VideoCapture& cap, ViewerOptions& opt) {
  for (int id = 0;; ++id) {
    if (id >= 8) break;
    if (!cap.open(id)) continue;
    bool match = false;
    if (opt.product_.has_value()) {
      if      (!strcasecmp(opt.product_.c_str(), "leap") && is_leap(cap)) match = true;
      else if (!strcasecmp(opt.product_.c_str(), "rigel") && is_rigel(cap)) match = true;
    } else if (is_leap(cap) || is_rigel(cap)) {
      match = true;
    }
    if (!match) {
      cap.release();
    } else {
      config_cap(librealuvc::VideoCapture& cap, ViewerOptions& opt)
      break;
    }
  }
}

void view_cap(librealuvc::VideoCapture& cap, ViewerOptions& opt) {
  for (int count = 0;; ++count) {
    ok = cap.read(mat);
    if (!ok) {
      D("cap.read() fail\n");
      break;
    }
    cv::imshow("cap_LR", mat);
    int c = (int)cv::waitKey(1);
    if ((c == 27) || (c == 'q')) break; // <ESC> or 'q' key
  }
}

extern "C"
int main(int argc, char* argv[]) {
  ViewerOptions options(argc, argv);
  try {
    librealuvc::VideoCapture cap;
    open_cap(cap, options);
    if (cap.isOpened()) {
      view_cap(cap, options);
    }
  } catch (std::exception e) {
	  fprintf(stderr, "ERROR: caught exception %s\n", e.what());
  }
  cv::destroyAllWindows();
  return 0;
}
