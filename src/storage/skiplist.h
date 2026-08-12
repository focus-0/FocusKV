#pragma once
#include <atomic>
#include <cstdlib>
#include <new>

#include "src/storage/arena.h"

namespace focuskv {

template <typename Key, class Comparator>
class SkipList {
 private:
  struct Node {
    Key const key;
    std::atomic<Node*> next[1];
  };

 public:
  explicit SkipList(Comparator cmp, Arena* arena);

  void Insert(const Key& key);
  bool Contains(const Key& key) const;

  class Iterator {
   public:
    explicit Iterator(const SkipList* list);
    bool Valid() const;
    const Key& key() const;
    void Next();
    void Seek(const Key& target);
    void SeekToFirst();

   private:
    const SkipList* list_;
    Node* node_;
  };

  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;

 private:
  enum { kMaxHeight = 12 };

  Node* NewNode(const Key& key, int height);
  int RandomHeight();
  Node* FindGreaterOrEqual(const Key& key, Node** predecessors) const;
  bool Equal(const Key& a, const Key& b) const { return compare_(a, b) == 0; }

  Comparator compare_;
  Arena* const arena_;
  Node* const head_;
  std::atomic<int> max_height_;
};

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::NewNode(
    const Key& key, int height) {
  size_t mem_size = sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1);
  char* mem = arena_->Allocate(mem_size);
  return new (mem) Node{key};
}

template <typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight() {
  int height = 1;
  while (height < kMaxHeight && ((std::rand() % 4) == 0)) {
    height++;
  }
  return height;
}

template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp, Arena* arena)
    : compare_(cmp), arena_(arena), head_(NewNode(Key(), kMaxHeight)), max_height_(1) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->next[i].store(nullptr, std::memory_order_relaxed);
  }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindGreaterOrEqual(
    const Key& key, Node** predecessors) const {
  Node* x = head_;
  int level = max_height_.load(std::memory_order_relaxed) - 1;

  while (true) {
    Node* next = x->next[level].load(std::memory_order_relaxed);
    if (next != nullptr && compare_(next->key, key) < 0) {
      x = next;
    } else {
      if (predecessors != nullptr) predecessors[level] = x;
      if (level == 0) return next;
      level--;
    }
  }
}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) const {
  Node* x = FindGreaterOrEqual(key, nullptr);
  return (x != nullptr && Equal(x->key, key));
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(const Key& key) {
  Node* predecessors[kMaxHeight];
  Node* x = FindGreaterOrEqual(key, predecessors);
  if (x != nullptr && Equal(x->key, key)) return;

  int height = RandomHeight();
  int max_h = max_height_.load(std::memory_order_relaxed);
  if (height > max_h) {
    for (int i = max_h; i < height; i++) {
      predecessors[i] = head_;
    }
    max_height_.store(height, std::memory_order_relaxed);
  }

  x = NewNode(key, height);
  for (int i = 0; i < height; i++) {
    x->next[i].store(predecessors[i]->next[i].load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
    predecessors[i]->next[i].store(x, std::memory_order_relaxed);
  }
}

// Iterator implementation
template <typename Key, class Comparator>
SkipList<Key, Comparator>::Iterator::Iterator(const SkipList* list)
    : list_(list), node_(nullptr) {}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Iterator::Valid() const {
  return node_ != nullptr;
}

template <typename Key, class Comparator>
const Key& SkipList<Key, Comparator>::Iterator::key() const {
  return node_->key;
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Iterator::Next() {
  node_ = node_->next[0].load(std::memory_order_relaxed);
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Iterator::Seek(const Key& target) {
  node_ = list_->FindGreaterOrEqual(target, nullptr);
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Iterator::SeekToFirst() {
  node_ = list_->head_->next[0].load(std::memory_order_relaxed);
}

}  // namespace focuskv