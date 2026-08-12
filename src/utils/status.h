#pragma once

#include <string>
#include "src/utils/slice.h"

namespace focuskv {

class Status {
 public:
  enum Code {
    kOk = 0,
    kNotFound = 1,
    kCorruption = 2,
    kIOError = 3,
    kInvalidArgument = 4
  };

  Status() : code_(kOk), msg_("") {}
  Status(Code code, const Slice& msg) : code_(code), msg_(msg.ToString()) {}

  static Status OK() { return Status(kOk, ""); }
  static Status NotFound(const Slice& msg = Slice()) { return Status(kNotFound, msg); }
  static Status Corruption(const Slice& msg = Slice()) { return Status(kCorruption, msg); }
  static Status IOError(const Slice& msg = Slice()) { return Status(kIOError, msg); }
  static Status InvalidArgument(const Slice& msg = Slice()) { return Status(kInvalidArgument, msg); }

  bool ok() const { return code_ == kOk; }
  bool IsNotFound() const { return code_ == kNotFound; }
  bool IsCorruption() const { return code_ == kCorruption; }
  bool IsIOError() const { return code_ == kIOError; }
  bool IsInvalidArgument() const { return code_ == kInvalidArgument; }

  Code code() const { return code_; }
  std::string ToString() const {
    if (ok()) return "OK";
    std::string result;
    switch (code_) {
      case kNotFound: result = "NotFound: "; break;
      case kCorruption: result = "Corruption: "; break;
      case kIOError: result = "IOError: "; break;
      case kInvalidArgument: result = "InvalidArgument: "; break;
      default: result = "Unknown code: "; break;
    }
    result.append(msg_);
    return result;
  }

 private:
  Code code_;
  std::string msg_;
};

}  // namespace focuskv
