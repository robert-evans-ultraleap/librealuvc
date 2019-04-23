// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include "drivers.h"
#include <librealuvc/ru_uvc.h>
#include <librealuvc/ru_videocapture.h>
#include <cstdio>

#if 1
#define D(...) { printf("DEBUG[%s,%d] ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#define D(...) { }
#endif

namespace librealuvc {

using std::shared_ptr;

static int32_t saturate(double val, int32_t min, int32_t max) {
  int32_t ival = (int32_t)val;
  if (ival < min) return min;
  if (ival > max) return max;
  return ival;
}

static int32_t flag(double val) {
  return ((val == 0.0) ? 0 : 1);
}

class PropertyDriverRigel : public IPropertyDriver {
 private:
  shared_ptr<uvc_device> dev_;
  double hdr_;
  double led_l_;
  double led_m_;
  double led_r_;
 public:
  PropertyDriverRigel(const shared_ptr<uvc_device>& dev) :
    dev_(dev),
    hdr_(0),
    led_l_(1),
    led_m_(1),
    led_r_(1) {
    D("PropertyDriverRigel::ctor() ...");
  }

  int get_frame_fixup() override {
    return 2;
  }
  
  HandlerResult get_prop(int prop_id, double* val) override {
    bool ok = false;
    int32_t ival = 0;
    switch (prop_id) {
      case cv::CAP_PROP_EXPOSURE:
        ok = dev_->get_pu(RU_OPTION_ZOOM_ABSOLUTE, ival);
        *val = (double)ival;
        break;
      case cv::CAP_PROP_GAIN:
        ok = dev_->get_pu(RU_OPTION_GAIN, ival);
        *val = (double)ival;
        break;
      case cv::CAP_PROP_GAMMA:
        ok = dev_->get_pu(RU_OPTION_GAMMA, ival);
        *val = (double)ival;
        break;
      case CAP_PROP_LEAP_HDR:
        *val = hdr_;
        return kHandlerTrue;
      case CAP_PROP_LEAP_LED_L:
        *val = led_l_;
        return kHandlerTrue;
      case CAP_PROP_LEAP_LED_M:
        *val = led_m_;
        return kHandlerTrue;
      case CAP_PROP_LEAP_LED_R:
        *val = led_r_;
        return kHandlerTrue;
      default:
        return kNotHandled;
    }
    return (ok ? kHandlerTrue : kHandlerFalse);
  }
  
  HandlerResult set_prop(int prop_id, double val) override {
    bool ok = false;
    switch (prop_id) {
      case cv::CAP_PROP_EXPOSURE:
        ok = dev_->set_pu(RU_OPTION_ZOOM_ABSOLUTE, saturate(val, 10, 0xffff));
        break;
      case cv::CAP_PROP_GAIN:
        ok = dev_->set_pu(RU_OPTION_GAIN, saturate(val, 16, 63));
        break;
      case cv::CAP_PROP_GAMMA:
        ok = dev_->set_pu(RU_OPTION_GAMMA, flag(val));
        break;
      case CAP_PROP_LEAP_HDR:
        hdr_ = val;
        ok = dev_->set_pu(RU_OPTION_CONTRAST, 0x0 | (flag(val)<<6));
        break;
      case CAP_PROP_LEAP_LED_L:
        led_l_ = val;
        ok = dev_->set_pu(RU_OPTION_CONTRAST, 0x2 | (flag(val)<<6));
        break;
      case CAP_PROP_LEAP_LED_M:
        led_l_ = val;
        ok = dev_->set_pu(RU_OPTION_CONTRAST, 0x3 | (flag(val)<<6));
        break;
      case CAP_PROP_LEAP_LED_R:
        led_l_ = val;
        ok = dev_->set_pu(RU_OPTION_CONTRAST, 0x4 | (flag(val)<<6));
        break;
      default:
        return kNotHandled;
    }
    return (ok ? kHandlerTrue : kHandlerFalse);
  }
};

#define VENDOR_RIGEL  0x2936
#define PRODUCT_RIGEL 0x1202

LIBREALUVC_EXPORT void import_driver_rigel() {
  register_property_driver(
    VENDOR_RIGEL,
    PRODUCT_RIGEL,
    [](const shared_ptr<uvc_device>& realuvc)->shared_ptr<IPropertyDriver> {
      return std::make_shared<PropertyDriverRigel>(realuvc);
    }
  );
}

} // end librealuvc
