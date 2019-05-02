#include "option_parse.h"
#include <librealuvc/ru_videocapture.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

#if 0
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define D(...) { }
#endif

#define VERSION "1.0.3 [2019-05-01]"

using namespace librealuvc;
using std::shared_ptr;
using std::string;
using std::weak_ptr;
using leap::Optional;
using leap::OptionParse;

class silent_exit : public std::exception {
 public:
  virtual const char* what() const noexcept override { return "SILENT_EXIT"; }
};

class ViewerOptions {
 public:
  Optional<int>    analog_gain_;
  Optional<int>    digital_gain_;
  Optional<int>    exposure_;
  Optional<double> fps_;
  Optional<bool>   gamma_;
  Optional<bool>   hdr_;
  Optional<int>    height_;
  Optional<bool>   leds_;
  Optional<int>    magnify_;
  Optional<int>    width_;
  Optional<string> product_;
 
 public:
  void usage(int line) {
    fprintf(
      stderr,
      "usage: viewer\n"
      "  --analog_gain <num>   analog gain\n"
      "  --digital_gain <num>  digital gain\n"
      "  --exposure <num>      exposure in microseconds\n"
      "  --fps <num>           frames-per-second\n"
      "  --gamma <on|off>      gamma-correction on or off (peripheral only)\n"
      "  --hdr <on|off>        high dynamic range on or off (peripheral only)\n"
      "  --height <num>        frame height in pixels\n"
      "  --leds <on|off>       led illumination on or off\n"
      "  --magnify <num>       magnify the on-screen image (without smoothing)\n"
      "  --product <string>    choose rigel or peripheral device\n"
      "  --width <num>         frame width in pixels\n"
    );
    throw silent_exit();
  }

#define USAGE() usage(__LINE__)

  ViewerOptions(int argc, char** argv) :
    leds_(true) {
    OptionParse p(argc, argv);
    while (!p.have_end()) {
      if (p.have_option("-?", "--help")) {
        USAGE();
      } else if (p.have_option_value("-a", "--analog_gain",  analog_gain_)) {
      } else if (p.have_option_value("-d", "--digital_gain", digital_gain_)) {
      } else if (p.have_option_value("-e", "--exposure",     exposure_)) {
      } else if (p.have_option_value("-f", "--fps",          fps_)) {
      } else if (p.have_option_value("-g", "--gamma",        gamma_)) {
      } else if (p.have_option_value("-Z", "--hdr",          hdr_)) {
      } else if (p.have_option_value("-h", "--height",       height_)) {
      } else if (p.have_option_value("-l", "--leds",         leds_)) {
      } else if (p.have_option_value("-m", "--magnify",      magnify_)) {
      } else if (p.have_option_value("-p", "--product",      product_)) {
      } else if (p.have_option_value("-w", "--width",        width_)) {
      } else {
        p.err_bad_option();
      }
    }
    if (p.is_fail()) USAGE();
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

bool is_rigel(shared_ptr<VideoCapture> cap) {
  return (cap->isOpened() && 
    (cap->get_vendor_id() == VENDOR_RIGEL) && 
    (cap->get_product_id() == PRODUCT_RIGEL));
}

bool is_periph(shared_ptr<VideoCapture> cap) {
  return (cap->isOpened() && 
    (cap->get_vendor_id() == VENDOR_LEAP) && 
    (cap->get_product_id() == PRODUCT_LEAP));
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

void config_cap(shared_ptr<VideoCapture> cap, ViewerOptions& opt) {
  Config* table = config_default;
  if      (is_periph(cap)) table = config_leap;
  else if (is_rigel(cap))  table = config_rigel;
  for (Config* p = table; p->width_ > 0; ++p) {
    if (opt.fps_.has_value()    && (opt.fps_ != p->fps_)) continue;
    if (opt.width_.has_value()  && (opt.width_ != p->width_)) continue;
    if (opt.height_.has_value() && (opt.height_ != p->height_)) continue;
    // Everything matched
    printf("-- set fps %.1f width %d height %d\n", p->fps_, p->width_, p->height_);
    fflush(stdout);
    cap->set(cv::CAP_PROP_FOURCC, str2fourcc("YUY2"));
    cap->set(cv::CAP_PROP_FPS,          p->fps_);
    cap->set(cv::CAP_PROP_FRAME_WIDTH,  p->width_);
    cap->set(cv::CAP_PROP_FRAME_HEIGHT, p->height_);
    // Other settings are napplied in init_trackbar_window
    if (opt.gamma_.has_value() && is_rigel(cap)) {
      printf("WARNING: gamma not supported on rigel hardware\n");
    }
    if (opt.hdr_.has_value() && is_rigel(cap)) {
      printf("WARNING: hdr not supported on rigel hardware\n");
    }
    return;
  }
  // Fall through without finding a matching (fps, width, height)
  fprintf(stderr, "ERROR: invalid fps, width, height\n");
  cap->release();
  throw silent_exit();
}

class SliderControl {
 public:
  std::weak_ptr<VideoCapture> cap_;
  int prop_id_;
  string name_;
  double min_;
  double max_;
  int want_;
};

class SliderWindow {
 public:
  string name_;
  std::vector<SliderControl> vec_;
};

SliderWindow sliders;

void on_trackbar_change(int /*val*/, void* arg) {
  SliderControl* s = (SliderControl*)arg;
  auto cap = s->cap_.lock();
  if (!cap) return;
  cap->set(s->prop_id_, (double)s->want_);
}

void init_trackbar_window(shared_ptr<VideoCapture> cap, ViewerOptions& opt) {
  char win_name[64];
  if      (is_rigel(cap))  strcpy(win_name, "Controls (rigel)");
  else if (is_periph(cap)) strcpy(win_name, "Controls (peripheral)");
  else                     strcpy(win_name, "Controls (camera)");
  cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);

  auto create_slider = [&](const char* name, int prop_id) {
    sliders.vec_.push_back(SliderControl());
    auto& s = sliders.vec_.back();
    s.cap_ = cap;
    s.prop_id_ = prop_id;
    s.name_ = name;
    s.max_ = -1.0;
    s.min_ = -1.0;
    bool have_range = cap->get_prop_range(prop_id, &s.min_, &s.max_);
    if (!have_range || (s.max_ <= s.min_)) {
      sliders.vec_.resize(sliders.vec_.size()-1);
      return;
    }
  };

  create_slider("analg_gain", cv::CAP_PROP_GAIN);
  create_slider("digtl_gain", cv::CAP_PROP_BRIGHTNESS);
  create_slider("exposure",   cv::CAP_PROP_EXPOSURE);
  if (is_periph(cap)) {
    create_slider("gamma", cv::CAP_PROP_GAMMA);
  }
  if (is_periph(cap) || is_rigel(cap)) {
    create_slider("hdr",  librealuvc::CAP_PROP_LEAP_HDR);
    create_slider("leds", librealuvc::CAP_PROP_LEAP_LEDS);
  }
  for (auto& s : sliders.vec_) {
    s.want_ = (int)((s.min_ + s.max_) / 2.0);
    switch (s.prop_id_) {
      case cv::CAP_PROP_GAIN:
        if (opt.analog_gain_.has_value()) s.want_ = opt.analog_gain_.value();
        break;
      case cv::CAP_PROP_BRIGHTNESS:
        if (opt.digital_gain_.has_value()) s.want_ = opt.digital_gain_.value();
        break;
      case cv::CAP_PROP_EXPOSURE:
        if (opt.exposure_.has_value()) s.want_ = opt.exposure_.value();
        break;
      case cv::CAP_PROP_GAMMA:
        s.want_ = 0;
        if (opt.gamma_.has_value()) s.want_ = (opt.gamma_.value() ? 1 : 0);
        break;
      case librealuvc::CAP_PROP_LEAP_HDR:
        s.want_ = 0;
        if (opt.hdr_.has_value()) s.want_ = (opt.hdr_.value() ? 1 : 0);
        break;
      case librealuvc::CAP_PROP_LEAP_LEDS:
        s.want_ = 1;
        if (opt.leds_.has_value()) s.want_ = (opt.leds_.value() ? 1 : 0);
        break;
      default:
        break;
    }
    cv::createTrackbar(s.name_, win_name, &s.want_, (int)(s.max_-s.min_), on_trackbar_change, &s);
    if (s.min_ != 0) {
      cv::setTrackbarMin(s.name_, win_name, (int)s.min_);
      cv::setTrackbarMax(s.name_, win_name, (int)s.max_);
    }
    on_trackbar_change(s.want_, &s);
  }
  cv::waitKey(1);
}

void open_cap(shared_ptr<VideoCapture> cap, ViewerOptions& opt) {
  int id;
  for (id = 0; id < 8; ++id) {
    if (!cap->open(id)) continue;
    if (opt.product_.has_value()) {
      if      (!strcmp(opt.product_.value().c_str(), "leap") && is_periph(cap)) break;
      else if (!strcmp(opt.product_.value().c_str(), "peripheral") && is_periph(cap)) break;
      else if (!strcmp(opt.product_.value().c_str(), "rigel") && is_rigel(cap)) break;
    } else if (is_periph(cap) || is_rigel(cap)) {
      break;
    }
    cap->release();
  }
  if (!cap->isOpened()) {
    const char* s = (opt.product_.has_value() ? opt.product_.value().c_str() : "any");
    fprintf(stderr, "ERROR: no camera device matching %s\n", s);
    throw silent_exit();
  }
}

#define ESC 27

void view_cap(shared_ptr<VideoCapture> cap, ViewerOptions& opt) {
  cv::Mat mat;
  cv::Mat magnified;
  int mag_factor = (opt.magnify_.has_value() ? opt.magnify_.value() : 1);
  bool stopped = false;
  for (int count = 0;; ++count) {
    bool ok = cap->read(mat);
    if (!ok) {
      fprintf(stderr, "ERROR: cap->read() failed\n");
      break;
    }
    if (!stopped) {
      if (mag_factor <= 1) {
        cv::imshow("cap_LR", mat);
      } else {
        double mag = (double)mag_factor;
        cv::resize(mat, magnified, cv::Size(), mag, mag, cv::INTER_NEAREST);
        char mag_name[32];
        sprintf(mag_name, "cap_LR_x%d", mag_factor);
        cv::imshow(mag_name, magnified);
      }
    }
    int c = (int)cv::waitKey(1);
    switch (c) {
      case 'g':
        if (stopped) {
          stopped = false;
          printf("-- go (enter 's' to stop)\n");
          fflush(stdout);
        }
        break;
      case 's':
        if (!stopped) {
          stopped = true;
          printf("-- stop (enter 'g' to go)\n");
          fflush(stdout);
        }
        break;
      case 'q':
      case ESC:
        printf("-- quit\n");
        fflush(stdout);
        return;
      default:
        if ((' ' <= c) && (c <= '~')) {
          printf("WARNING: key '%c' ignored, use 'g'=go, 's'=stop, 'q'=quit\n", c);
          fflush(stdout);
        }
        break;
    }
  }
}

extern "C"
int main(int argc, char* argv[]) {
  printf("viewer: Leap Motion peripheral/rigel viewer %s\n", VERSION);
  fflush(stdout);
  ViewerOptions options(argc, argv);
  try {
    auto cap = std::make_shared<VideoCapture>();
    open_cap(cap, options);
    if (cap->isOpened()) {
      config_cap(cap, options);
      init_trackbar_window(cap, options);
    }
    if (cap->isOpened()) {
      view_cap(cap, options);
    }
  } catch (std::exception e) {
    if (strcmp(e.what(), "SILENT_EXIT") != 0) {
	    fprintf(stderr, "ERROR: caught exception %s\n", e.what());
    }
  }
  cv::destroyAllWindows();
  return 0;
}
