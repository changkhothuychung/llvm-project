// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s



namespace Example1 {
  struct Cat {
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
    operator int() = delete;
  };

  X x(Y{});
} // namespace Example2

namespace Example4 {
  // the converting constructor Cat(const Dog&) is selected.
  struct Dog;
  struct Cat {
    Cat(const Dog&);
  };

  struct Dog {
    operator Cat() = delete;
  };

  Cat cat(Dog{});
} // namespace Example4

namespace Example5 {
  // A2(const A1&) is selected.
  struct A1 {};

  struct A2 {
    A2(const A1&);
    A2(const A2&);
  };

  struct B : A1 {
    operator A2() = delete;
  };

  A2 a(B{});
} // namespace Example5

namespace Example6 {
  // S::operator T& is selected
  struct T {
    T(T const&);
  };

  struct S {
    operator T() = delete;
    operator T&();
  };

  S s;
  T t(s);
} // namespace Example6

namespace Example7 {
  // this should be well-formed in clang now
  // before this, it was an ambiguity
  struct Y;

  struct X {
    X(const Y&);
  };

  struct A {
    operator X();
  };

  struct B {
    operator X();
  };

  struct Y : A, B { };

  X x(Y{});
} // namespace Example7

namespace Example8 {
  template <int i>
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

// Tests that fail and will issue diagnostics.

namespace NoViableConversion {
  template <int i>
  struct T {
    T(const T&) requires(i != 0); // expected-note {{candidate constructor not viable: constraints not satisfied}} \
                                  // expected-note {{because '0 != 0' (0 != 0) evaluated to false}}
  };

  struct S {
    operator int(); // no operator T<0>() at all
  };

  T<0> t(S{}); // expected-error {{no matching constructor for initialization of 'T<0>'}}
} // namespace NoViableConversion

namespace AmbiguousConversion {
  template <int i>
  struct T {
    T(const T&) requires(i != 0);
  };

  struct S1 {
    operator T<0>(); // expected-note {{candidate function}}
  };

  struct S2 {
    operator T<0>(); // expected-note {{candidate function}}
  };

  struct U : S1, S2 {};

  T<0> t(U{}); // expected-error {{call to constructor of 'T<0>' is ambiguous}}
} // namespace AmbiguousConversion

namespace DeletedConversion {
  template <int i>
  struct T {
    T(const T&) requires(i != 0);
  };

  struct S {
    operator T<0>() = delete; // expected-note {{'operator T' has been explicitly marked deleted here}}
  };

  T<0> t(S{}); // expected-error {{call to deleted constructor of 'T<0>'}}
} // namespace DeletedConversion

namespace InaccessibleConversion {
  template <int i>
  struct T {
    T(const T&) requires(i != 0);
  };

  struct S {
  private:
    operator T<0>(); // expected-note {{declared private here}}
  };

  T<0> t(S{}); // expected-error {{'operator T' is a private member of 'InaccessibleConversion::S'}}
} // namespace InaccessibleConversion
