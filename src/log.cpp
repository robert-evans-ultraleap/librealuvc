#include <cstdio>
#include <mutex>

#include "types.h"

namespace librealuvc {
  
namespace { // anon

class single_logger {
 public:
  std::mutex mutex_;
  ru_severity sev_console_;
  ru_severity sev_file_;
  FILE* log_;
 
 public:
  single_logger() :
    mutex_(),
    sev_console_(RU_SEVERITY_ERROR),
    sev_file_(RU_SEVERITY_NONE),
    log_(nullptr) {
  }
  
  void log_to_file(ru_severity min_sev, const char* file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_) fclose(log_);
    log_ = fopen(file_path, "w");
    if (!log_) {
      fprintf(stderr, "ERROR: can't write to logfile \"%s\"\n", file_path);
      exit(1);
    }
    sev_file_ = min_sev;
  }
  
  static const char* sev2str(ru_severity sev) {
    switch (sev) {
      case RU_SEVERITY_DEBUG:   return "DEBUG";
      case RU_SEVERITY_INFO:    return "INFO";
      case RU_SEVERITY_WARNING: return "WARNING";
      case RU_SEVERITY_ERROR:   return "ERROR";
      case RU_SEVERITY_FATAL:   return "FATAL";
      default: return "UNKNOWN";
    }
  }
  
  void log_msg(ru_severity sev, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool want_console = (sev >= sev_console_);
    bool want_file = ((sev >= sev_file_) && log_);
    if (!want_console && !want_file) return;
    auto sev_str = sev2str(sev);
    if (want_console) {
      printf("%s: librealuvc: ", sev_str);
      fwrite(msg.data(), 1, msg.length(), stdout);
      fflush(stdout);
    }
    if (want_file) {
      fprintf(log_, "%s: librealuvc: ", sev_str);
      fwrite(msg.data(), 1, msg.length(), log_);
      fflush(log_);
    }
  }
};

single_logger* get_single_logger() {
  static single_logger single;
  return &single;
}

} // end anon

void log_to_console(ru_severity min_sev) {
  get_single_logger()->sev_console_ = min_sev;
}

void log_to_file(ru_severity min_sev, const char* file_path) {
  get_single_logger()->log_to_file(min_sev, file_path);
}

void log_msg(ru_severity sev, const std::string& msg) {
  get_single_logger()->log_msg(sev, msg);
}

}
