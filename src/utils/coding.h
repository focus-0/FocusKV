#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include "src/utils/slice.h"

namespace focuskv {

// Encoding to std::string
void PutFixed32(std::string* dst, uint32_t value);
void PutFixed64(std::string* dst, uint64_t value);

// Encoding to raw char* buffer
inline void EncodeFixed32(char* dst, uint32_t value) {
  std::memcpy(dst, &value, sizeof(value));
}

inline void EncodeFixed64(char* dst, uint64_t value) {
  std::memcpy(dst, &value, sizeof(value));
}

inline uint32_t DecodeFixed32(const char* ptr) {
  uint32_t result;
  std::memcpy(&result, ptr, sizeof(result));
  return result;
}

inline uint64_t DecodeFixed64(const char* ptr) {
  uint64_t result;
  std::memcpy(&result, ptr, sizeof(result));
  return result;
}

void PutLengthPrefixedSlice(std::string* dst, const Slice& value);
bool GetLengthPrefixedSlice(Slice* input, Slice* result);

}  // namespace focuskv
