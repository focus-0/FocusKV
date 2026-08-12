#pragma once
#include <atomic>   // For std::atomic - enables lock-free, thread-safe pointer updates
                    // Atomic operations ensure multiple threads can safely read/write
                    // without traditional mutex locks (much faster!)
#include <cstdlib>  // For rand() - used to randomly determine node heights
#include <new>      // For placement new operator - allows constructing objects
                    // at a specific memory address (allocated by Arena)

#include "src/storage/arena.h"

namespace focuskv {

// SkipList: A probabilistic data structure that allows O(log n) search/insert
// It's like a linked list with express lanes - taller nodes skip over more elements
template <typename Key>
class SkipList {
 public:
  // 'explicit' keyword prevents implicit conversions from Arena* to SkipList
  // Example: Without explicit, this would compile: SkipList<int> list = arena_ptr;
  // With explicit, you MUST write: SkipList<int> list(arena_ptr);
  // This prevents accidental conversions and makes code more clear
  explicit SkipList(Arena* arena);
  
  // Insert a new key into the skiplist
  void Insert(const Key& key);
  
  // Check if a key exists in the skiplist
  // 'const' means this method doesn't modify the skiplist
  bool Contains(const Key& key) const;
  
  // Disable copy constructor and copy assignment operator
  // SkipList manages pointers and memory, copying would be complex and dangerous
  // (would need deep copy of all nodes, careful handling of arena, etc.)
  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;

 private:
  // Node structure: Each node in the skiplist
  struct Node {
    Key const key;  // The key stored in this node (immutable after creation)
    
    // Flexible Array Member pattern:
    // - We declare next[1] but actually allocate more space than this
    // - std::atomic<Node*> ensures thread-safe pointer updates without locks
    // - A node with height H will have H pointers: next[0], next[1], ..., next[H-1]
    // - next[0] = lowest level (links all nodes)
    // - next[H-1] = highest level (skips many nodes)
    // - The actual size is determined at runtime in NewNode()
    std::atomic<Node*> next[1];
  };
  
  // Maximum height for any node in the skiplist
  // Using enum instead of const/constexpr is a common C++ practice for integer constants
  // Benefits: compile-time constant, no storage needed, can't take address
  enum { kMaxHeight = 12 };

  // Allocate a new node with the given key and height
  // Uses arena for memory and placement new to construct the Node
  Node* NewNode(const Key& key, int height);
  
  // Randomly determine the height for a new node
  // Uses coin-flip logic: 1/4 chance to grow taller
  // This randomness ensures balanced O(log n) performance
  int RandomHeight();
  
  // Find the first node with key >= target key
  // If predecessors is not null, fills it with the path taken (used for Insert)
  // Returns the node found, or nullptr if all nodes are smaller
  Node* FindGreaterOrEqual(const Key& key, Node** predecessors) const;
  
  // Pointer to the arena allocator (immutable - can't point to different arena)
  Arena* const arena_;
  
  // Dummy head node that simplifies edge cases (immutable pointer)
  // Always exists, has height kMaxHeight, and comes before all real nodes
  Node* const head_;
  
  // Current maximum height of any node in the list
  // Atomic because multiple threads might read/write during concurrent inserts
  std::atomic<int> max_height_;
};

// ================================================================================
// IMPLEMENTATION
// ================================================================================

// NewNode: Allocate and construct a new skiplist node
template <typename Key>
typename SkipList<Key>::Node* SkipList<Key>::NewNode(const Key& key, int height) {
  // Calculate memory needed:
  // - sizeof(Node) includes the struct + next[0] (one pointer)
  // - We need (height - 1) additional pointers for next[1], next[2], ..., next[height-1]
  // - Each pointer is std::atomic<Node*>
  size_t mem_size = sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1);
  
  // Get raw memory from the arena (just a char* pointing to uninitialized memory)
  char* mem = arena_->Allocate(mem_size);
  
  // Placement new: Construct a Node object at the memory address 'mem'
  // Syntax: new (address) Type(constructor_args)
  // This doesn't allocate new memory - it just calls the constructor at 'mem'
  // The Node constructor will initialize 'key' and the first next pointer
  return new (mem) Node(key);
}

// RandomHeight: Determine height for a new node using coin-flip logic
template <typename Key>
int SkipList<Key>::RandomHeight() {
  int height = 1;  // Every node has at least height 1
  
  // Coin flip: 1/4 (25%) chance to grow taller
  // This probability distribution ensures:
  // - About 75% of nodes have height 1
  // - About 18.75% have height 2
  // - About 4.7% have height 3
  // - And so on...
  // This creates the "express lane" structure for fast searches
  while (height < kMaxHeight && ((std::rand() % 4) == 0)) {
    height++;
  }
  return height;
}

// Constructor: Initialize an empty skiplist
template <typename Key>
SkipList<Key>::SkipList(Arena* arena)
    : arena_(arena),                      // Store the arena pointer
      head_(NewNode(Key(), kMaxHeight)),  // Create dummy head with max height
      max_height_(1) {                    // Start with effective height of 1
  
  // Initialize all of head's next pointers to nullptr
  // The head node doesn't store real data - it's just a starting point
  for (int i = 0; i < kMaxHeight; i++) {
    head_->next[i].store(nullptr, std::memory_order_relaxed);
  }
}

// FindGreaterOrEqual: Core search algorithm
// Traverses from top-left to bottom-right, following the "express lanes"
template <typename Key>
typename SkipList<Key>::Node* SkipList<Key>::FindGreaterOrEqual(
    const Key& key, Node** predecessors) const {
  Node* x = head_;  // Start at the dummy head
  
  // Start at the highest active level (max_height_ - 1)
  int level = max_height_.load(std::memory_order_relaxed) - 1;

  while (true) {
    // Look at the next node at the current level
    Node* next = x->next[level].load(std::memory_order_relaxed);
    
    // Decide: go forward or go down?
    if (next != nullptr && next->key < key) {
      // Next node exists and is too small -> move forward at this level
      x = next;
    } else {
      // Next node is >= key (or doesn't exist) -> we've gone as far as we can at this level
      
      // If caller wants the path (for Insert), save this node as predecessor
      if (predecessors != nullptr) predecessors[level] = x;

      // If we're at the bottom level, return what we found
      if (level == 0) return next;
      
      // Otherwise, drop down one level and continue searching
      level--;
    }
  }
}

// Contains: Check if a key exists in the skiplist
template <typename Key>
bool SkipList<Key>::Contains(const Key& key) const {
  // Find the first node >= key
  Node* x = FindGreaterOrEqual(key, nullptr);
  
  // It's a match only if the node exists AND its key equals our search key
  return (x != nullptr && x->key == key);
}

// Insert: Add a new key to the skiplist
template <typename Key>
void SkipList<Key>::Insert(const Key& key) {
  // Step 1: Find where to insert and record the path (predecessors)
  // predecessors[i] = the node that should point to our new node at level i
  Node* predecessors[kMaxHeight];
  Node* x = FindGreaterOrEqual(key, predecessors);
  
  // Optional: Don't insert duplicates
  // (Some skiplist implementations allow duplicates - depends on use case)
  if (x != nullptr && x->key == key) return;
  
  // Step 2: Randomly determine the height of the new node
  int height = RandomHeight();
  int max_h = max_height_.load(std::memory_order_relaxed);
  
  // Step 3: If new node is taller than current max, update max_height_
  if (height > max_h) {
    // For levels above the old max, the predecessor is the head node
    for (int i = max_h; i < height; i++) {
      predecessors[i] = head_;
    }
    // Update the global max height
    // Note: In a fully concurrent skiplist, we'd use CAS (compare-and-swap) here
    max_height_.store(height, std::memory_order_relaxed);
  }
  
  // Step 4: Create the new node
  x = NewNode(key, height);
  
  // Step 5: Link the new node into the skiplist at all levels
  for (int i = 0; i < height; i++) {
    // Standard linked list insertion:
    // 1. New node points to what predecessor was pointing to
    x->next[i].store(predecessors[i]->next[i].load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
    // 2. Predecessor now points to new node
    predecessors[i]->next[i].store(x, std::memory_order_relaxed);
  }
}

}  // namespace focuskv
