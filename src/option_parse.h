#ifndef LIBREALUVC_OPTIONPARSE_H
#define LIBREALUVC_OPTIONPARSE_H 1
//
// option_parse.h - easy-to-use command-line parser
//
// Richard Cownie, Leap Motion, 2019-04-18.
//
// Copyright (c) Leap Motion 2019.  All rights reserved.
//
#include <sstream>
#include <string>

namespace leap {

using std::string;

// std::optional<T> wouold be good, but that's only in C++17

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

class OptionParse {
 public:
  int    argc_;
  char** argv_;
  int    idx_;
  char*  pos_;
  int    nerr_;

 public:
  OptionParse(int argc, char** argv);

 private:
  bool advance();

 public:
  void err_bad_option();
  
  void err_no_value(const char* sname, const char* lname);

  template<typename T>
  bool have_value(T& val);
  
  template<typename T>
  bool have_value(Optional<T>& val) {
    T tmp;
    if (!have_value(tmp)) return false;
    val = tmp;
    return true;
  };

  bool have_option(const char* short_name, const char* long_name);

  template<typename T>
  bool have_option_value(const char* sname, const char* lname, T& val) {
    if (!have_option(sname, lname)) return false;
    if (!have_value(val)) { err_no_value(sname, lname); return true; }
    return true;
  }

  bool have_end();
  
  bool is_fail();
};

template<> bool OptionParse::have_value<bool>(bool& val);
template<> bool OptionParse::have_value<double>(double& val);
template<> bool OptionParse::have_value<int>(int& val);
template<> bool OptionParse::have_value<string>(string& val);

} // end leap

#endif