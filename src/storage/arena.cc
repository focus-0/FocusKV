#include "src/storage/arena.h"

#include <algorithm>
#include <cstdlib>

namespace focuskv  // lets say in future I add a library which also has arena
                   // class,  hence by wrapping it in focuskv we can avoid name
                   // collisions
{
static const int kBlockSize = 4096;
// Function1
Arena::Arena() : alloc_ptr_(nullptr), alloc_limit_(nullptr) {}

// Function2
Arena::~Arena() {
  for (char* block : blocks_) {
    delete[] block;
  }
}

// Function 3
char* Arena::Allocate(size_t bytes) {
  if (alloc_ptr_ + bytes <= alloc_limit_) {
    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    return result;
  }
  return AllocateFallback(bytes);
}
// Function 4
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
// Function 5
char* Arena::AllocateNewBlock(size_t block_bytes) {
  char* result = new char[block_bytes];
  blocks_.push_back(result);

  return result;
}

}  // namespace focuskv
