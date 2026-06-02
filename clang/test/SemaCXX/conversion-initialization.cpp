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

namespace Example4 {
  // the converting constructor Cat(const Dog&) is selected.
  struct Dog;
  struct Cat {
    Cat(const Dog&);
  };

  struct Dog {
    operator Cat();
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
    operator A2();
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

// Tests that fail and will issue diagnostics.

namespace Step3_NoViableCandidate {
  struct T {
    T() = default; // expected-note {{candidate constructor not viable: requires 0 arguments, but 1 was provided}}
    T(const T&) = delete; // expected-note {{candidate constructor not viable: no known conversion from 'S' to 'const T' for 1st argument}}
    T(T&&) = delete; // expected-note {{candidate constructor not viable: no known conversion from 'S' to 'T' for 1st argument}}
  };

  struct S {
    operator int(); // no operator T() at all
  };

  T t(S{}); // expected-error {{no viable conversion function from 'S' to 'T'}}
} // namespace Step3_NoViableCandidate

namespace Step3_Ambiguous {
  struct T {
    T() = default;
    T(const T&) = delete; // expected-note {{candidate constructor has been explicitly deleted}}
    T(T&&) = delete; // expected-note {{candidate constructor has been explicitly deleted}}
  };

  struct S1 {
    operator T();
  };

  struct S2 {
    operator T();
  };

  struct U : S1, S2 {};

  T t(U{}); // expected-error {{call to constructor of 'T' is ambiguous}}
} // namespace Step3_Ambiguous

namespace Step3_Deleted {
  struct T {
    T() = default;
    T(const T&) = delete;
    T(T&&) = delete;
  };

  struct S {
    operator T() = delete; // expected-note {{'operator T' has been explicitly marked deleted here}}
  };

  T t(S{}); // expected-error {{conversion from 'S' to 'T' invokes a deleted function}}
} // namespace Step3_Deleted

namespace Step3_Inaccessible {
  struct T {
    T() = default;
    T(const T&) = delete;
    T(T&&) = delete;
  };

  struct S {
  private:
    operator T(); // expected-note {{declared private here}}
  };

  T t(S{}); // expected-error {{'operator T' is a private member of 'Step3_Inaccessible::S'}}
} // namespace Step3_Inaccessible
