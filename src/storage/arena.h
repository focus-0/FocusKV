#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace focuskv  // lets say in future I add a library which also has arena
                   // class,  hence by wrapping it in focuskv we can avoid name
                   // collisions
{
class Arena {
 private:
  std::vector<char*> blocks_;
  // Function4
  char* AllocateFallback(size_t bytes);
  // Function 5
  char* AllocateNewBlock(size_t bytes);
  char* alloc_ptr_;  // Pointer to the next available byte in the current block
  char* alloc_limit_;  // Pointer to the very end of the current block

 public:
  // Function 1
  Arena();
  // Function 3
  char* Allocate(size_t bytes);
  Arena(const Arena&) = delete;  // disabling the copy constructor
  Arena& operator=(const Arena&) =
      delete;  // disabling the assignment constructor
  // Function 2
  ~Arena();
};
}  // namespace focuskv
