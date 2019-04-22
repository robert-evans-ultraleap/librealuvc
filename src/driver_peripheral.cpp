// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include "drivers.h"
#include <librealuvc/ru_uvc.h>
#include <librealuvc/ru_videocapture.h>

namespace librealuvc {

using std::shared_ptr;

class PropertyDriverPeripheral : public IPropertyDriver {
 public:
  PropertyDriverPeripheral(const shared_ptr<uvc_device>&) {
  }

  int get_frame_fixup() override {
    return 1;
  }

  HandlerResult get_prop(int prop_id, double* val) override {
    return kNotHandled;
  }
  
  HandlerResult set_prop(int prop_id, double val) override {
    return kNotHandled;
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
