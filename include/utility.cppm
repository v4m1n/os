module;

#include <atomic>

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
  explicit shared_ptr(T *p) : cnt_(new std::atomic<uint64_t>(1)), ptr_(p) {
  }
  explicit shared_ptr(const shared_ptr<T> &p) noexcept : cnt_(p.cnt_), ptr_(p.ptr_) {
    if (p.cnt_) ++(*p.cnt_);
  }
  explicit shared_ptr(shared_ptr<T> &&p) noexcept : cnt_(p.cnt_), ptr_(p.ptr_) {
    p.cnt_ = nullptr;
    p.ptr_ = nullptr;
  }
  shared_ptr& operator=(T *p) noexcept {
    dec();
    cnt_ = new std::atomic<uint64_t>(1);
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

};
