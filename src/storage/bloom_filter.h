#pragma once

#include <algorithm>
#include <string>

#include "src/utils/slice.h"

namespace focuskv {

class BloomFilter {
 public:
  void Init(size_t expected_keys) {
    num_bits_ = std::max<size_t>(64, expected_keys * 10);
    bits_.assign((num_bits_ + 7) / 8, 0);
  }

  void Add(const Slice& key) {
    for (size_t i = 0; i < kNumHashes; ++i) {
      SetBit(Hash(key, i));
    }
  }

  bool MayContain(const Slice& key) const {
    if (bits_.empty()) return true;
    for (size_t i = 0; i < kNumHashes; ++i) {
      if (!GetBit(Hash(key, i))) return false;
    }
    return true;
  }

  std::string Serialize() const { return bits_; }

  void Deserialize(const Slice& data) {
    bits_.assign(data.data(), data.size());
    num_bits_ = bits_.size() * 8;
  }

  bool empty() const { return bits_.empty(); }

 private:
  static const size_t kNumHashes = 2;

  static uint32_t Hash(const Slice& key, size_t seed) {
    uint32_t h = static_cast<uint32_t>(seed * 0x9e3779b9);
    for (size_t i = 0; i < key.size(); ++i) {
      h ^= static_cast<uint8_t>(key.data()[i]);
      h *= 0x01000193;
    }
    return h;
  }

  void SetBit(uint32_t hash) {
    size_t bit = hash % num_bits_;
    bits_[bit / 8] |= static_cast<char>(1 << (bit % 8));
  }

  bool GetBit(uint32_t hash) const {
    size_t bit = hash % num_bits_;
    return (bits_[bit / 8] & (1 << (bit % 8))) != 0;
  }

  std::string bits_;
  size_t num_bits_{0};
};

}  // namespace focuskv
