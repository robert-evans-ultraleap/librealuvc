
#include <librealuvc/ru_videocapture.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#if 1
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define D(...) { }
#endif

using namespace librealuvc;
using std::string;

// std::optional<T> doesn't arrive until C++17 :-(

template<typename T>
class Optional {
 private:
  bool has_value_;
  T value_;
 
 public:
  Optional() : has_value_(false) { }
  Optional(const T& value) : has_value_(true), value_(value) { }
  Optional& operator=(const T& value) {
    has_value_ = true;
    value_ = value;
    return *this;
  }
  
  bool operator==(const T& b) const { return (value_ == b); }
  bool operator!=(const T& b) const { return (value_ != b); }
  
  bool has_value() const { return has_value_; }
  
  const T& value() const { return value_; }
};

static bool strcasediff(const char* pa, const char* pb) {
  for (;;) {
    char a = *pa;
    char b = *pb;
    if ((a == 0) && (b == 0)) return false;
    if (('A' <= a) && (a <= 'Z')) a += 'a'-'A';
    if (('A' <= b) && (b <= 'Z')) b += 'a'-'A';
    if (a != b) return true;
  }
}

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
        ++idx_;
        pos_ = &a[2];
      } else {
        ++idx_;
        pos_ = ((idx_ < argc_) ? argv_[idx_] : nullptr);
      }
      D("have_option short_name %s pos_ %s", short_name, pos_ ? pos_ : "null");
      return true;
    } else if (!strcmp(a, long_name)) {
      ++idx_;
      pos_ = ((idx_ < argc_) ? argv_[idx_] : nullptr);
      D("have_option long_name %s pos_ %s", long_name, pos_ ? pos_ : "null");
      return true;
    }
    return false;
  }
  
  bool have_bool(bool* val) {
    if (!pos_) return false;
    if (!strcasediff(pos_, "false") ||
        !strcasediff(pos_, "off") ||
        !strcasediff(pos_, "no")) {
      *val = false;
      return advance();
    }
    if (!strcasediff(pos_, "true") ||
        !strcasediff(pos_, "on") ||
        !strcasediff(pos_, "yes")) {
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
    D("have_int %d length %d", *val, endp-pos_);
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

class ViewerOptions {
 public:
  Optional<string> product_;
  Optional<double> fps_;
  Optional<int> height_;
  Optional<int> width_;
 
 public:
  void usage(int line) {
    D("usage(%d)", line);
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
      D("argv[%d] = '%s'", p.idx_, p.argv_[p.idx_]);
      if        (p.have_option("-?", "--help")) {
        usage(__LINE__);
      } else if (p.have_option("-f", "--fps")) {
        if (!p.have_double(&dval)) usage(__LINE__);
        fps_ = dval;
      } else if (p.have_option("-h", "--height")) {
        if (!p.have_int(&ival)) usage(__LINE__);
        height_ = ival;
      } else if (p.have_option("-p", "--product")) {
        if (!p.have_string(&sval)) usage(__LINE__);
        product_ = sval;
      } else if (p.have_option("-w", "--width")) {
        if (!p.have_int(&ival)) usage(__LINE__);
        width_ = ival;
      } else {
        fprintf(stderr, "ERROR: unexpected command-line option '%s'\n", p.argv_[p.idx_]);
        usage(__LINE__);
      }
    }
  }

};

static uint32_t str2fourcc(const char* s) {
  uint32_t result = 0;
  for (int j = 0; j < 4; ++j) {
    uint32_t b = ((int)s[j] & 0xff);
    result |= (b << 8*(3-j));
  }
  return result;
}

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
  {  50.0, 752, 480 },
  {  57.5, 640, 480 },
  { 100.0, 752, 240 },
  { 115.0, 640, 240 },
  { 190.0, 752, 120 },
  { 214.0, 640, 120 },
  { 0, 0, 0}
};

Config config_rigel[] = {
  {  24.0, 800, 800 },
  {  45.0, 640, 480 },
  {  70.0, 680, 286 },
  {  85.0, 400, 400 },
  {  90.0, 384, 384 },
  { 100.0, 368, 368 },
  { 0, 0, 0 }
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
    D("set fps %.1f width %d height %d", p->fps_, p->width_, p->height_);
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
  int id;
  for (id = 0; id < 8; ++id) {
    D("cap.open(%d) ...", id);
    if (!cap.open(id)) continue;
    if (opt.product_.has_value()) {
      if      (!strcasediff(opt.product_.value().c_str(), "leap") && is_leap(cap)) break;
      else if (!strcasediff(opt.product_.value().c_str(), "rigel") && is_rigel(cap)) break;
    } else if (is_leap(cap) || is_rigel(cap)) {
      break;
    }
    cap.release();
  }
  if (!cap.isOpened()) {
    const char* s = (opt.product_.has_value() ? opt.product_.value().c_str() : "any");
    fprintf(stderr, "ERROR: no camera device matching %s\n", s);
    exit(1);
  }
}

void view_cap(librealuvc::VideoCapture& cap, ViewerOptions& opt) {
  cv::Mat mat;
  for (int count = 0;; ++count) {
    bool ok = cap.read(mat);
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
      config_cap(cap, options);
    }
    if (cap.isOpened()) {
      view_cap(cap, options);
    }
  } catch (std::exception e) {
	  fprintf(stderr, "ERROR: caught exception %s\n", e.what());
  }
  cv::destroyAllWindows();
  return 0;
}
