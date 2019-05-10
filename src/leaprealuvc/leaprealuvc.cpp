// bindings.cpp : Defines the exported functions for the DLL application.
//

#include <librealuvc/ru_videocapture.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdio>
#include <iostream>
#include "leaprealuvc.h"

using namespace librealuvc;

#if 0
#define D(...) { }
#else
#define D(...) { printf("DEBUG[%d] ", __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#endif

int triple_input(int input)
{
  std::cout << "Hi!\n";
  return input * 3;
}

static VideoCapture *_cap = NULL;

// -------

#define VENDOR_RIGEL       0x2936
#define VENDOR_MSI         0x5986
#define VENDOR_LOGITECH    0x046d
#define VENDOR_LEAP        0xf182

#define PRODUCT_RIGEL      0x1202
#define PRODUCT_GS63CAM    0x0547
#define PRODUCT_C905       0x080a
#define PRODUCT_LEAP       0x0003

bool is_rigel(VideoCapture *cap) {
  return (cap->isOpened() &&
    (cap->get_vendor_id() == VENDOR_RIGEL) &&
    (cap->get_product_id() == PRODUCT_RIGEL));
}

bool is_periph(VideoCapture *cap) {
  return (cap->isOpened() &&
    (cap->get_vendor_id() == VENDOR_LEAP) &&
    (cap->get_product_id() == PRODUCT_LEAP));
}

void open_cap(VideoCapture *cap) {
  int id;
  for (id = 0; id < 8; ++id) {
    printf("Trying %d\n", id);
    if (!cap->open(id)) { continue; }
    printf("Checking vendor_id of %d\n", id);
    if (is_periph(cap) || is_rigel(cap)) { break; }
    printf("Releasing %d\n", id);
    if (cap->isOpened()) { cap->release(); }
  }
  if (!cap->isOpened()) {
    fprintf(stderr, "ERROR: No matching camera device\n");
    throw std::exception("No matching camera device.");
  }
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

static uint32_t str2fourcc(const char* s) {
  uint32_t result = 0;
  for (int j = 0; j < 4; ++j) {
    uint32_t b = ((int)s[j] & 0xff);
    result |= (b << 8 * (3 - j));
  }
  return result;
}

void config_cap(VideoCapture *cap) {
  Config* table = config_default;
  if (is_periph(cap)) table = config_leap;
  else if (is_rigel(cap))  table = config_rigel;
  printf("HELLO");
  for (Config* p = table; p->width_ > 0; ++p) {
    // Everything matched
    printf("-- set fps %.1f width %d height %d\n", p->fps_, p->width_, p->height_);
    fflush(stdout);
    cap->set(cv::CAP_PROP_FOURCC, str2fourcc("YUY2"));
    cap->set(cv::CAP_PROP_FPS, p->fps_);
    cap->set(cv::CAP_PROP_FRAME_WIDTH, p->width_);
    cap->set(cv::CAP_PROP_FRAME_HEIGHT, p->height_);
    // Other settings are napplied in init_trackbar_window
    //if (opt.gamma_.has_value() && is_rigel(cap)) {
    //	printf("WARNING: gamma not supported on rigel hardware\n");
    //}
    //if (opt.hdr_.has_value() && is_rigel(cap)) {
    //	printf("WARNING: hdr not supported on rigel hardware\n");
    //}
    return;
  }
  // Fall through without finding a matching (fps, width, height)
  fprintf(stderr, "ERROR: invalid fps, width, height\n");
  cap->release();
  throw std::exception("invalid fps, width, height");
}

void require_capturing(VideoCapture *cap) {
  if (!cap->isOpened()) {
    open_cap(cap);
    printf("Finished open_cap\n");
    config_cap(cap);
    printf("Finished config_cap\n");
  }
}

// -------

uint32_t try_get_image(uint8_t *write_addr, uint32_t size) {
  try {
    printf("the write addr is %d\n", write_addr);
    if (_cap == NULL) { _cap = new librealuvc::VideoCapture(); }
    if (_cap == NULL) { throw "Unable to initialize VideoCapture."; }
    require_capturing(_cap);

    cv::Mat mat;
    bool ok = _cap->read(mat);
    if (!ok) {
      throw std::exception("Failed to read capture image!");
    }
    //int size = image.total() * image.elemSize();
    //byte * bytes = new byte[size];  // you will have to delete[] that later
    //std::memcpy(bytes, image.data, size * sizeof(byte));
    int captureSize = mat.total() * mat.elemSize();
    if (size != captureSize) {
      printf("Provided bytes size %d does not match capture bytes size %d.\n", size, captureSize);
      throw std::exception("Image bytes don't match request bytes in size.");
    }
    std::memcpy(write_addr, mat.data, size * sizeof(uint8_t));
    printf("Wrote image bytes!\n");

    //std::memcpy(bytes, image.data, size * sizeof(byte));

    printf("Releasing...\n");
    if (_cap != NULL) {
      _cap->release();
    }

    printf("Returning...\n");
    return 0;
  }
  catch (std::exception e) {
    printf("ERROR: Caught exception: %s\n", e.what());
  }
  //cv::destroyAllWindows();
  return 1;
}
