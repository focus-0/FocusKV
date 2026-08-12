#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace focuskv {

class Arena {
 private:
  std::vector<char*> blocks_;
  char* AllocateFallback(size_t bytes);
  char* AllocateNewBlock(size_t bytes);
  char* alloc_ptr_;
  char* alloc_limit_;
  std::atomic<size_t> memory_usage_{0};

 public:
  Arena();
  char* Allocate(size_t bytes);
  size_t MemoryUsage() const { return memory_usage_.load(std::memory_order_relaxed); }
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  ~Arena();
};

}  // namespace focuskv
