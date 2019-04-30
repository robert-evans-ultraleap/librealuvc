// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include "drivers.h"
#include <librealuvc/ru_uvc.h>
#include <librealuvc/ru_videocapture.h>

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

class PropertyDriverPeripheral : public IPropertyDriver {
 private:
  shared_ptr<uvc_device> dev_;
  double hdr_;
  double leds_;
 public:
  PropertyDriverPeripheral(const shared_ptr<uvc_device>& dev) :
    dev_(dev),
    hdr_(0),
    leds_(1) {
  }

  int get_frame_fixup() override {
    return 1;
  }

  HandlerResult get_prop(int prop_id, double* val) override {
    bool ok = true;
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
        break;
      case CAP_PROP_LEAP_LEDS:
        *val = leds_;
        break;
      default:
        return kHandlerNotDone;
    }
    return (ok ? kHandlerTrue : kHandlerFalse);
  }
  
  HandlerResult get_prop_range(int prop_id, double* min_val, double* max_val) override {
    bool ok = true;
    *min_val = 0;
    *max_val = 0;
    switch (prop_id) {
      case cv::CAP_PROP_EXPOSURE:
        *min_val = 10;
        *max_val = 300;
        break;
      case cv::CAP_PROP_GAIN:
        *min_val = 16;
        *max_val = 63;
        break;
      case cv::CAP_PROP_GAMMA:
        *max_val = 1;
        break;
      case CAP_PROP_LEAP_HDR:
        *max_val = 1;
        break;
      case CAP_PROP_LEAP_LEDS:
        *max_val = 1;
        break;
      default:
        return kHandlerNotDone;
    }
    return (ok ? kHandlerTrue : kHandlerFalse);
  }
  
  HandlerResult set_prop(int prop_id, double val) override {
    bool ok = true;
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
      case CAP_PROP_LEAP_LEDS:
        ok &= dev_->set_pu(RU_OPTION_CONTRAST, 0x2 | (flag(val)<<6));
        ok &= dev_->set_pu(RU_OPTION_CONTRAST, 0x3 | (flag(val)<<6));
        ok &= dev_->set_pu(RU_OPTION_CONTRAST, 0x4 | (flag(val)<<6));
        if (ok) leds_ = val;
        break;
      default:
        return kHandlerNotDone;
    }
    return (ok ? kHandlerTrue : kHandlerFalse);
  }
};

#define VENDOR_PERIPHERAL  0xf182
#define PRODUCT_PERIPHERAL 0x0003

LIBREALUVC_EXPORT void import_driver_peripheral() {
  register_property_driver(
    VENDOR_PERIPHERAL,
    PRODUCT_PERIPHERAL,
    [](const shared_ptr<uvc_device>& realuvc)->shared_ptr<IPropertyDriver> {
      return std::make_shared<PropertyDriverPeripheral>(realuvc);
    }
  );
}

} // end librealuvc
