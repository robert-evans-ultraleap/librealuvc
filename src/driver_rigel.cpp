// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Leap Motion Corporation. All Rights Reserved.

#include "drivers.h"
#include <librealuvc/ru_uvc.h>
#include <librealuvc/ru_videocapture.h>

namespace librealuvc {

using std::string;
using std::shared_ptr;

class PropertyDriverRigel : public IPropertyDriver {
 public:
  PropertyDriverRigel(const shared_ptr<uvc_device>&) {
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
