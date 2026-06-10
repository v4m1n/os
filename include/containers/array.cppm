module;
#include "stddef.h"

export module array;
import kmm;
import debug;
import utility;
export import knew;


export template <typename T>
class array {
  size_t size_;
  T *data_;
  
public:
  template <typename ...Args>
  array(size_t size, Args... args) {
    data_ = reinterpret_cast<T *>(kmm::kmalloc(sizeof(T)*size));
    size_ = size;
    for (size_t i = 0; i < size_; ++i) {
      new (&data_[i]) T(args...);
    }
  }

  array() : size_(0), data_(0) {
  }

  T& at(size_t i) {
    dbg::panic_assert(i < size_, "array access out of bounds\n");
    return data_[i];
  }
  T* data() {
    return data_;
  }
  size_t size() {
    return size_;
  }
  void resize(size_t new_size) {
    auto new_data = reinterpret_cast<T *>(kmm::kmalloc(sizeof(T)*new_size));

    for (size_t i = 0; i < size_ && i < new_size; ++i) {
      new (&new_data[i]) T(move(data_[i]));
    }

    if (size_ < new_size) {
      for (size_t i = size_; i < new_size; ++i) {
        new (&new_data[i]) T();
      }
    }
    else {
      for (size_t i = new_size; i < size_; ++i) {
        data_[i].~T();
      }
    }

    kmm::kfree(data_);
    size_ = new_size;
    data_ = new_data;
  }
  template <typename ...Args>
  void emplace_back(Args... args) {
    auto new_size = size_+1;
    auto new_data = reinterpret_cast<T *>(kmm::kmalloc(sizeof(T)*new_size));

    for (size_t i = 0; i < size_; ++i) {
      new (&new_data[i]) T(move(data_[i]));
    }

    new (&new_data[size_]) T(args...);

    kmm::kfree(data_);
    size_ = new_size;
    data_ = new_data;
  }

  bool empty() {
    return !size_;
  }
  T *begin() {
    return data_;
  }
  T *end() {
    return data_+size_;
  }
  const T *cbegin() {
    return data_;
  }
  const T *cend() {
    return data_+size_;
  }
};
