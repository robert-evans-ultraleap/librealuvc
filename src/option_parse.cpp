//
// option_parse.cpp - easy-to-use command-line parser
//
// Richard Cownie, Leap Motion, 2019-04-18
//
// Copyright (c) Leap Motion 2019.  All rights reserved.
//
#include "option_parse.h"
#include <cstdio>

namespace leap {

OptionParse::OptionParse(int argc, char** argv) :
  argc_(argc),
  argv_(argv),
  idx_(1),
  pos_(nullptr),
  nerr_(0) {
}

bool OptionParse::advance() {
  if (pos_ && (pos_ == argv_[idx_])) ++idx_;
  pos_ = nullptr;
  return true;
}

void OptionParse::err_bad_option() {
  fprintf(stderr, "ERROR: option '%s' not recognized\n",
    (idx_ < argc_) ? argv_[idx_] : "<nullptr>");
  ++idx_;
  ++nerr_;
}

void OptionParse::err_no_value(const char* sname, const char* lname) {
  fprintf(stderr, "ERROR: option \"%s\" or \"%s\" has no value\n", sname, lname);
  ++nerr_;
}

bool OptionParse::have_option(const char* short_name, const char* long_name) {
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
    return true;
  } else if (!strcmp(a, long_name)) {
    ++idx_;
    pos_ = ((idx_ < argc_) ? argv_[idx_] : nullptr);
    return true;
  }
  return false;
}

static bool strcasediff(const char* pa, const char* pb) {
  for (;;) {
    char a = *pa++;
    char b = *pb++;
    if ((a == 0) && (b == 0)) return false;
    if (('A' <= a) && (a <= 'Z')) a += 'a'-'A';
    if (('A' <= b) && (b <= 'Z')) b += 'a'-'A';
    if (a != b) return true;
  }
}

template<> 
bool OptionParse::have_value(bool& val) {
  if (!pos_) return false;
  if (!strcasediff(pos_, "false") ||
      !strcasediff(pos_, "off") ||
      !strcasediff(pos_, "no")) {
    val = false;
    return advance();
  }
  if (!strcasediff(pos_, "true") ||
      !strcasediff(pos_, "on") ||
      !strcasediff(pos_, "yes")) {
    val = true;
    return advance();
  }
  return false;
};

template<> 
bool OptionParse::have_value(double& val) {
  if (!pos_) return false;
  char* endp = pos_;
  val = strtod(pos_, &endp);
  return ((endp > pos_) && (*endp == 0) && advance());
};

template<> 
bool OptionParse::have_value(int& val) {
  if (!pos_) return false;
  char* endp = pos_;
  val = (int)strtol(pos_, &endp, 0);
  return ((endp > pos_) && (*endp == 0) && advance());
};

template<> 
bool OptionParse::have_value(string& val) {
  if (!pos_) return false;
  val = std::string(pos_);
  return ((val.length() > 0) && advance());
};

bool OptionParse::have_end() {
  return (idx_ >= argc_);
}

bool OptionParse::is_fail() {
  return (nerr_ > 0);
}

} // end leap
