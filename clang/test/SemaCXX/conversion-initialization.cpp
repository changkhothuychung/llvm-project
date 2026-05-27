// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s



namespace Example1 {
  struct Cat {
    Cat() = default;
    Cat(const Cat&) = delete;
    Cat(Cat&&) = delete;
  };

  struct Dog {
    operator Cat();
  };

  Dog d;
  Cat c(d);
  Cat c1{d};
  Cat c2 = {d};
} // namespace Example1

namespace Example1_With_CvQualified {
  struct T {
    T() = default;
    T(const T&) = delete;
    T(T&&) = delete;
  };

  struct S {
    operator const T();
  };

  T t(S{});
} // namespace Example1_With_CvQualified

namespace Example1_With_PrivateConstructors {
  struct T {
    private:
      T() = default;
      T(const T&) = delete;
      T(T&&) = delete;
  };

  struct S {
    operator const T();
  };

  T t(S{});
} // namespace Example1_With_PrivateConstructors

namespace Example1_With_ReturnsReference {
  // This test should fail since it does not create any temporary to elide.
  struct T {
    T() = default;
    T(const T&) = delete; // expected-note {{marked deleted here}}
    T(T&&) = delete;
  };

  struct S {
    operator T&();
  };

  T t(S{}); // expected-error {{call to deleted constructor of 'T'}}
} // namespace Example1_With_ReturnsReference

namespace ReturnsDerived {
  // This test should fail.
  struct Base {
    Base() = default;
    Base(const Base&) = delete;
    Base(Base&&) = delete; // expected-note {{marked deleted here}}
  };

  struct Derived : Base {};

  struct S {
    operator Derived();
  };

  Base b(S{}); // expected-error {{call to deleted constructor of 'Base'}}
} // namespace ReturnsDerived

namespace Example2 {
  struct X {
    X(int);
    // X(X&&); // implicitly declared
  };

  struct Y {
    operator X();
    operator int();
  };

  X x(Y{});
} // namespace Example2

namespace Example8 {
  template <int i = 0>
  class NonCopyable {
  public:
    NonCopyable(const NonCopyable&) requires(i != 0);
  private:
    NonCopyable(int x);
    friend struct Source;
  };

  struct Source {
    operator NonCopyable<0>();
  };

  NonCopyable<0> nc(Source{}); // OK, calls Source::operator NonCopyable<0>()
} // namespace Example8
