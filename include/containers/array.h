#pragma once
#include "kmm.h"
#include "stddef.h"
#include "new.h"
#include "debug.h"

template <typename T>
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

