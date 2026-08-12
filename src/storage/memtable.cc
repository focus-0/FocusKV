#include "src/storage/memtable.h"
#include <algorithm>

namespace focuskv {

static const char* GetLengthPrefixedSliceData(const char* ptr, Slice* slice) {
  if (ptr == nullptr) {
    *slice = Slice();
    return nullptr;
  }
  uint32_t len = DecodeFixed32(ptr);
  *slice = Slice(ptr + sizeof(uint32_t), len);
  return ptr + sizeof(uint32_t) + len;
}

int KeyComparator::operator()(const char* aptr, const char* bptr) const {
  if (aptr == bptr) return 0;
  if (aptr == nullptr) return -1;
  if (bptr == nullptr) return 1;

  Slice a, b;
  GetLengthPrefixedSliceData(aptr, &a);
  GetLengthPrefixedSliceData(bptr, &b);

  Slice a_user_key(a.data(), a.size() - 8);
  Slice b_user_key(b.data(), b.size() - 8);

  int r = a_user_key.compare(b_user_key);
  if (r != 0) return r;

  uint64_t a_num = DecodeFixed64(a.data() + a.size() - 8);
  uint64_t b_num = DecodeFixed64(b.data() + b.size() - 8);

  if (a_num > b_num) return -1;
  if (a_num < b_num) return +1;
  return 0;
}

MemTable::MemTable() : comparator_(), arena_(), table_(comparator_, &arena_) {
  Ref();
}

void MemTable::Add(uint64_t seq, ValueType type, const Slice& key, const Slice& value) {
  size_t key_size = key.size();
  size_t val_size = value.size();
  size_t internal_key_size = key_size + 8;
  size_t encoded_len = sizeof(uint32_t) + internal_key_size + sizeof(uint32_t) + val_size;

  char* buf = arena_.Allocate(encoded_len);
  char* p = buf;

  // 1. Encode InternalKey length into raw buffer
  EncodeFixed32(p, static_cast<uint32_t>(internal_key_size));
  p += sizeof(uint32_t);

  // 2. Encode UserKey bytes
  std::memcpy(p, key.data(), key_size);
  p += key_size;

  // 3. Encode (SeqNum << 8 | ValueType)
  uint64_t packed = (seq << 8) | static_cast<uint8_t>(type);
  EncodeFixed64(p, packed);
  p += sizeof(uint64_t);

  // 4. Encode Value length
  EncodeFixed32(p, static_cast<uint32_t>(val_size));
  p += sizeof(uint32_t);

  // 5. Encode Value bytes
  std::memcpy(p, value.data(), val_size);

  table_.Insert(buf);
}

bool MemTable::Get(const Slice& key, std::string* value, Status* status) {
  size_t key_size = key.size();
  size_t internal_key_size = key_size + 8;

  // Allocate lookup buffer
  std::string lookup_buf;
  PutFixed32(&lookup_buf, static_cast<uint32_t>(internal_key_size));
  lookup_buf.append(key.data(), key_size);
  uint64_t max_packed = (0x00FFFFFFFFFFFFFFULL << 8) | static_cast<uint8_t>(kTypeValue);
  PutFixed64(&lookup_buf, max_packed);

  SkipList<const char*, KeyComparator>::Iterator iter(&table_);
  iter.Seek(lookup_buf.data());

  if (iter.Valid()) {
    const char* entry = iter.key();
    Slice internal_key;
    const char* val_ptr = GetLengthPrefixedSliceData(entry, &internal_key);

    Slice user_key(internal_key.data(), internal_key.size() - 8);
    if (user_key == key) {
      uint64_t tag = DecodeFixed64(internal_key.data() + internal_key.size() - 8);
      ValueType type = static_cast<ValueType>(tag & 0xff);
      if (type == kTypeValue) {
        Slice val;
        GetLengthPrefixedSliceData(val_ptr, &val);
        value->assign(val.data(), val.size());
        *status = Status::OK();
        return true;
      } else if (type == kTypeDeletion) {
        *status = Status::NotFound();
        return true;
      }
    }
  }
  return false;
}

}  // namespace focuskv
