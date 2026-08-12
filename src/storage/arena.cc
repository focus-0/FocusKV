#include "src/storage/arena.h"
#include <algorithm>
#include <cstdlib>

namespace focuskv {

static const int kBlockSize = 4096;

Arena::Arena() : alloc_ptr_(nullptr), alloc_limit_(nullptr) {
  memory_usage_.store(0, std::memory_order_relaxed);
}

Arena::~Arena() {
  for (char* block : blocks_) {
    delete[] block;
  }
}

char* Arena::Allocate(size_t bytes) {
  if (alloc_ptr_ + bytes <= alloc_limit_) {
    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    return result;
  }
  return AllocateFallback(bytes);
}

char* Arena::AllocateFallback(size_t bytes) {
  if (bytes > kBlockSize / 4) {
    return AllocateNewBlock(bytes);
  }

  alloc_ptr_ = AllocateNewBlock(kBlockSize);
  alloc_limit_ = alloc_ptr_ + kBlockSize;

  char* result = alloc_ptr_;
  alloc_ptr_ += bytes;
  return result;
}

char* Arena::AllocateNewBlock(size_t block_bytes) {
  char* result = new char[block_bytes];
  blocks_.push_back(result);
  memory_usage_.fetch_add(block_bytes + sizeof(char*), std::memory_order_relaxed);
  return result;
}

}  // namespace focuskv
