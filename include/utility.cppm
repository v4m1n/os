module;

#include <atomic>
#include <cstddef>
#include <cstdint>

export module utility;
import knew;
import debug;

export template<class T1, class T2>
struct pair {
  T1 first;
  T2 second;
};

export template<class T> struct remove_reference { typedef T type; };
template<class T> struct remove_reference<T&> { typedef T type; };
template<class T> struct remove_reference<T&&> { typedef T type; };

export template<class T>
using remove_reference_t = typename remove_reference<T>::type;

export template< class T >
constexpr remove_reference_t<T>&& move(T&& t) noexcept {
  return static_cast<typename remove_reference<T>::type&&>(t);
}
export namespace util {

template <typename T>
class shared_ptr {
  std::atomic<uint64_t> *cnt_;
  T *ptr_;

  void dec() noexcept {
    if (ptr_ && --(*cnt_) == 0) {
      delete cnt_;
      delete ptr_;
    }
  }

  
public:
  constexpr shared_ptr() noexcept : cnt_(nullptr), ptr_(nullptr) {
  }
  constexpr shared_ptr(std::nullptr_t) noexcept : cnt_(nullptr), ptr_(nullptr) {
  }
  shared_ptr(T *p) : cnt_(new std::atomic<uint64_t>(1)), ptr_(p) {
  }
  shared_ptr(const shared_ptr<T> &p) noexcept : cnt_(p.cnt_), ptr_(p.ptr_) {
    if (p.cnt_) ++(*p.cnt_);
  }
  shared_ptr(shared_ptr<T> &&p) noexcept : cnt_(p.cnt_), ptr_(p.ptr_) {
    p.cnt_ = nullptr;
    p.ptr_ = nullptr;
  }
  shared_ptr& operator=(T *p) noexcept {
    if (p) {
      dec();
      cnt_ = new std::atomic<uint64_t>(1);
    }
    else {
      cnt_ = nullptr;
    }
    ptr_ = p;
    return *this;
  }
  shared_ptr& operator=(const shared_ptr<T> &p) noexcept {
    dec();
    cnt_ = p.cnt_;
    ptr_ = p.ptr_;
    if (p.cnt_) ++(*p.cnt_);
    return *this;
  }
  shared_ptr& operator=(shared_ptr<T> &&p) {
    dec();
    cnt_ = p.cnt_;
    ptr_ = p.ptr_;
    p.cnt_ = nullptr;
    p.ptr_ = nullptr;
    return this;
  }
  ~shared_ptr() noexcept {
    dec();
  }

  uint64_t use_cnt() {
    return cnt_->load();
  }
  T *get() {
    return ptr_;
  }
  T* operator->() const noexcept {
    return ptr_;
  }
  T& operator*() const noexcept {
    return *ptr_;
  }
  explicit operator bool() const noexcept {
    return ptr_;
  }
  auto operator<=>(const shared_ptr<T>&) const = default;
};

template <typename K>
struct hash {
  uint64_t operator()(const K& key) const {
    uint64_t k = static_cast<uint64_t>(key);
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
};

template <typename T>
struct hash<T*> {
  uint64_t operator()(T* key) const {
    uint64_t k = reinterpret_cast<uint64_t>(key);
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
};

template <typename K, typename V, typename Hash = hash<K>>
class hash_map {
  struct Entry {
    K key;
    V value;
    bool occupied = false;
    bool deleted = false;
  };

  Entry* table_ = nullptr;
  size_t capacity_ = 0;
  size_t size_ = 0;
  size_t deleted_count_ = 0;
  Hash hasher_;

  void resize(size_t new_capacity) {
    Entry* old_table = table_;
    size_t old_capacity = capacity_;
    
    table_ = new Entry[new_capacity];
    capacity_ = new_capacity;
    size_ = 0;
    deleted_count_ = 0;
    
    for (size_t i = 0; i < old_capacity; ++i) {
      if (old_table[i].occupied && !old_table[i].deleted) {
        insert(old_table[i].key, old_table[i].value);
      }
    }
    delete[] old_table;
  }

public:
  hash_map(size_t initial_capacity = 16) {
    capacity_ = initial_capacity;
    table_ = new Entry[capacity_];
  }
  
  ~hash_map() {
    delete[] table_;
  }

  hash_map(const hash_map& other) = delete;
  hash_map& operator=(const hash_map& other) = delete;

  hash_map(hash_map&& other) noexcept : table_(other.table_), capacity_(other.capacity_), size_(other.size_), deleted_count_(other.deleted_count_) {
    other.table_ = nullptr;
    other.capacity_ = 0;
    other.size_ = 0;
    other.deleted_count_ = 0;
  }

  hash_map& operator=(hash_map&& other) noexcept {
    if (this != &other) {
      delete[] table_;
      table_ = other.table_;
      capacity_ = other.capacity_;
      size_ = other.size_;
      deleted_count_ = other.deleted_count_;
      other.table_ = nullptr;
      other.capacity_ = 0;
      other.size_ = 0;
      other.deleted_count_ = 0;
    }
    return *this;
  }

  void insert(const K& key, const V& value) {
    if (capacity_ == 0 || size_ + deleted_count_ >= capacity_ / 2) {
      resize(capacity_ == 0 ? 16 : capacity_ * 2);
    }
    
    size_t idx = hasher_(key) % capacity_;
    size_t first_deleted = capacity_;

    while (table_[idx].occupied) {
      if (!table_[idx].deleted && table_[idx].key == key) {
        table_[idx].value = value;
        return;
      }
      if (table_[idx].deleted && first_deleted == capacity_) {
        first_deleted = idx;
      }
      idx = (idx + 1) % capacity_;
    }

    if (first_deleted != capacity_) {
      idx = first_deleted;
      deleted_count_--;
    }
    
    table_[idx].key = key;
    table_[idx].value = value;
    table_[idx].occupied = true;
    table_[idx].deleted = false;
    size_++;
  }

  V& operator[](const K& key) {
    if (capacity_ == 0 || size_ + deleted_count_ >= capacity_ / 2) {
      resize(capacity_ == 0 ? 16 : capacity_ * 2);
    }
    
    size_t idx = hasher_(key) % capacity_;
    size_t first_deleted = capacity_;

    while (table_[idx].occupied) {
      if (!table_[idx].deleted && table_[idx].key == key) {
        return table_[idx].value;
      }
      if (table_[idx].deleted && first_deleted == capacity_) {
        first_deleted = idx;
      }
      idx = (idx + 1) % capacity_;
    }

    if (first_deleted != capacity_) {
      idx = first_deleted;
      deleted_count_--;
    }
    
    table_[idx].key = key;
    table_[idx].value = V{};
    table_[idx].occupied = true;
    table_[idx].deleted = false;
    size_++;
    return table_[idx].value;
  }

  V* find(const K& key) {
    if (capacity_ == 0) return nullptr;
    size_t idx = hasher_(key) % capacity_;
    size_t start_idx = idx;

    while (table_[idx].occupied) {
      if (!table_[idx].deleted && table_[idx].key == key) {
        return &table_[idx].value;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx) break;
    }
    return nullptr;
  }

  bool erase(const K& key) {
    if (capacity_ == 0) return false;
    size_t idx = hasher_(key) % capacity_;
    size_t start_idx = idx;

    while (table_[idx].occupied) {
      if (!table_[idx].deleted && table_[idx].key == key) {
        table_[idx].deleted = true;
        size_--;
        deleted_count_++;
        return true;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx) break;
    }
    return false;
  }

  size_t size() const { return size_; }
  void clear() {
    if (capacity_ == 0) return;
    for (size_t i = 0; i < capacity_; ++i) {
      table_[i].occupied = false;
      table_[i].deleted = false;
    }
    size_ = 0;
    deleted_count_ = 0;
  }
};

};
