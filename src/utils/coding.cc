#include "src/utils/coding.h"

namespace focuskv {

void PutFixed32(std::string* dst, uint32_t value) {
  char buf[sizeof(value)];
  std::memcpy(buf, &value, sizeof(value));
  dst->append(buf, sizeof(buf));
}

void PutFixed64(std::string* dst, uint64_t value) {
  char buf[sizeof(value)];
  std::memcpy(buf, &value, sizeof(value));
  dst->append(buf, sizeof(buf));
}

void PutLengthPrefixedSlice(std::string* dst, const Slice& value) {
  PutFixed32(dst, static_cast<uint32_t>(value.size()));
  dst->append(value.data(), value.size());
}

bool GetLengthPrefixedSlice(Slice* input, Slice* result) {
  if (input->size() < sizeof(uint32_t)) return false;
  uint32_t len = DecodeFixed32(input->data());
  input->remove_prefix(sizeof(uint32_t));
  if (input->size() < len) return false;
  *result = Slice(input->data(), len);
  input->remove_prefix(len);
  return true;
}

}  // namespace focuskv
