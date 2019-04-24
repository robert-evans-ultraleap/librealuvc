#include "option_parse.h"
#include <librealuvc/ru_videocapture.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#if 0
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define D(...) { }
#endif

#define VERSION "1.0.2 [2019-04-18]"

using namespace librealuvc;
using std::string;
using leap::Optional;
using leap::OptionParse;

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
    D("usage(%d)", line);
    fprintf(
      stderr,
      "usage: viewer\n"
      "  --analog_gain <num>   analog gain 16-63\n"
      "  --digital_gain <num>  digital gain\n"
      "  --exposure <num>      exposure in microseconds\n"
      "  --fps <num>           frames-per-second\n"
      "  --gamma <on|off>      gamma-correction on or off\n"
      "  --hdr <on|off>        high dynamic range on or off\n"
      "  --height <num>        frame height in pixels\n"
      "  --leds <on|off>.....  led illumination on or off\n"
      "  --product <string>    choose rigel or peripheral device\n"
      "  --width <num>         frame width in pixels\n"
    );
    throw std::exception("SILENT_EXIT");
  }

#define USAGE() usage(__LINE__)

  ViewerOptions(int argc, char** argv) {
    OptionParse p(argc, argv);
    while (!p.have_end()) {
      D("argv[%d] = '%s'", p.idx_, p.argv_[p.idx_]);
      if (p.have_option("-?", "--help")) {
        USAGE();
      } else if (p.have_option_value("-a", "--analog_gain",  analog_gain_)) {
      } else if (p.have_option_value("-d", "--digital_gain", digital_gain_)) {
      } else if (p.have_option_value("-e", "--exposure",     exposure_)) {
      } else if (p.have_option_value("-f", "--fps",          fps_)) {
      } else if (p.have_option_value("-g", "--gamma",        gamma_)) {
      } else if (p.have_option_value("-z", "--hdr",          hdr_)) {
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
    printf("-- set fps %.1f width %d height %d\n", p->fps_, p->width_, p->height_);
    fflush(stdout);
    cap.set(cv::CAP_PROP_FOURCC, str2fourcc("YUY2"));
    cap.set(cv::CAP_PROP_FPS,          p->fps_);
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  p->width_);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, p->height_);
    //
    // Other settings from command-line options.
    //
    if (opt.analog_gain_.has_value()) {
      cap.set(cv::CAP_PROP_GAIN, opt.analog_gain_.value());
    }
    if (opt.digital_gain_.has_value()) {
      cap.set(cv::CAP_PROP_BRIGHTNESS, opt.digital_gain_.value());
    }
    if (opt.exposure_.has_value()) {
      int val = opt.exposure_.value();
      if (val < 10) val = 10;
      if (val > 0xffff) val = 0xffff;
      cap.set(cv::CAP_PROP_EXPOSURE, val);
    }
    if (opt.gamma_.has_value()) {
      cap.set(cv::CAP_PROP_GAMMA, opt.gamma_.value() ? 1 : 0);
    }
    if (opt.hdr_.has_value()) {
      cap.set(librealuvc::CAP_PROP_LEAP_HDR, opt.hdr_.value() ? 1 : 0);
    }
    if (opt.leds_.has_value()) {
      double val = (opt.leds_.value() ? 1 : 0);
      cap.set(librealuvc::CAP_PROP_LEAP_LED_L, val);
      cap.set(librealuvc::CAP_PROP_LEAP_LED_M, val);
      cap.set(librealuvc::CAP_PROP_LEAP_LED_R, val);
    }
    return;
  }
  // Fall through without finding a matching (fps, width, height)
  fprintf(stderr, "ERROR: invalid fps, width, height\n");
  cap.release();
  throw std::exception("SILENT_EXIT");
}

void open_cap(librealuvc::VideoCapture& cap, ViewerOptions& opt) {
  int id;
  for (id = 0; id < 8; ++id) {
    D("cap.open(%d) ...", id);
    if (!cap.open(id)) continue;
    if (opt.product_.has_value()) {
      if      (!strcmp(opt.product_.value().c_str(), "leap") && is_leap(cap)) break;
      else if (!strcmp(opt.product_.value().c_str(), "peripheral") && is_leap(cap)) break;
      else if (!strcmp(opt.product_.value().c_str(), "rigel") && is_rigel(cap)) break;
    } else if (is_leap(cap) || is_rigel(cap)) {
      break;
    }
    cap.release();
  }
  if (!cap.isOpened()) {
    const char* s = (opt.product_.has_value() ? opt.product_.value().c_str() : "any");
    fprintf(stderr, "ERROR: no camera device matching %s\n", s);
    throw std::exception("SILENT_EXIT");
  }
}

#define ESC 27

void view_cap(librealuvc::VideoCapture& cap, ViewerOptions& opt) {
  cv::Mat mat;
  cv::Mat magnified;
  int mag_factor = (opt.magnify_.has_value() ? opt.magnify_.value() : 1);
  bool stopped = false;
  for (int count = 0;; ++count) {
    bool ok = cap.read(mat);
    if (!ok) {
      D("cap.read() fail\n");
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
  printf("viewer: Leap Motion leap/rigel viewer %s\n", VERSION);
  fflush(stdout);
  ViewerOptions options(argc, argv);
  try {
    librealuvc::VideoCapture cap;
    open_cap(cap, options);
    if (cap.isOpened()) {
      config_cap(cap, options);
    }
    if (cap.isOpened()) {
      D("view_cap() ...");
      view_cap(cap, options);
      D("view_cap() done");
    }
  } catch (std::exception e) {
    D("caught exception %s", e.what());
    if (strcmp(e.what(), "SILENT_EXIT") != 0) {
	    fprintf(stderr, "ERROR: caught exception %s\n", e.what());
    }
  }
  D("cv::destroyAllWindows() ...");
  cv::destroyAllWindows();
  return 0;
}
