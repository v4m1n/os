export module utility;

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
