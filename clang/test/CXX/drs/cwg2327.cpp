// RUN: %clang_cc1 -std=c++98 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,cxx98-14
// RUN: %clang_cc1 -std=c++11 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,cxx98-14
// RUN: %clang_cc1 -std=c++14 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,cxx98-14
// RUN: %clang_cc1 -std=c++17 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx17
// RUN: %clang_cc1 -std=c++20 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx17,since-cxx20
// RUN: %clang_cc1 -std=c++23 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx17,since-cxx20
// RUN: %clang_cc1 -std=c++2c %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx17,since-cxx20

// cxx98-14-no-diagnostics

namespace cwg2327 { // cwg2327: 23
// Copy elision for direct-initialization with a conversion function.
// See also P2828R2.

#if __cplusplus >= 201703L
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
#if __cplusplus >= 202002L
  // In C++17, [dcl.init.aggr] treats Cat as an aggregate because a
  // deleted-on-first-declaration copy/move ctor is user-declared but not
  // user-provided; brace-init from a single element then triggers the
  // "excess elements in struct initializer" diagnostic before Rule 1 has
  // a chance to run. P1008 (C++20) tightens the rule to "no user-declared
  // constructors", making Cat non-aggregate and letting Rule 1 elide the
  // copy/move ctor in the list-init cases too.
  Cat c1{d};
  Cat c2 = {d};
#endif
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
    T(const T&) = delete; // since-cxx17-note {{marked deleted here}}
    T(T&&) = delete;
  };

  struct S {
    operator T&();
  };

  T t(S{}); // since-cxx17-error {{call to deleted constructor of 'T'}}
} // namespace Example1_With_ReturnsReference

namespace ReturnsDerived {
  // This test should fail.
  struct Base {
    Base(const Base&) = delete;
    Base(Base&&) = delete; // since-cxx17-note {{marked deleted here}}
  };

  struct Derived : Base {};

  struct S {
    operator Derived();
  };

  Base b(S{}); // since-cxx17-error {{call to deleted constructor of 'Base'}}
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

namespace ExplicitConversionInCopyListInit {
  // CWG2327 / P2828R3 [over.match.list]/1: in copy-list-initialization, if the
  // second-phase conversion-initialization fallback selects an explicit
  // conversion function, the initialization is ill-formed.
  struct T {
    T(int);
  };

  struct S {
    explicit operator T(); // since-cxx17-note {{explicit conversion function declared here}}
  };

  S s;
  T t1{s};    // OK: direct-list-init permits explicit conversion functions.
  T t2 = {s}; // since-cxx17-error {{chosen conversion function is explicit in copy-list-initialization}}
} // namespace ExplicitConversionInCopyListInit
#endif // __cplusplus >= 201703L

#if __cplusplus >= 202002L
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
    T(const T&) requires(i != 0); // since-cxx20-note {{candidate constructor not viable: constraints not satisfied}} \
                                  // since-cxx20-note {{because '0 != 0' (0 != 0) evaluated to false}}
  };

  struct S {
    operator int(); // no operator T<0>() at all
  };

  T<0> t(S{}); // since-cxx20-error {{no matching constructor for initialization of 'T<0>'}}
} // namespace NoViableConversion

namespace AmbiguousConversion {
  template <int i>
  struct T {
    T(const T&) requires(i != 0);
  };

  struct S1 {
    operator T<0>(); // since-cxx20-note {{candidate function}}
  };

  struct S2 {
    operator T<0>(); // since-cxx20-note {{candidate function}}
  };

  struct U : S1, S2 {};

  T<0> t(U{}); // since-cxx20-error {{call to constructor of 'T<0>' is ambiguous}}
} // namespace AmbiguousConversion

namespace DeletedConversion {
  template <int i>
  struct T {
    T(const T&) requires(i != 0);
  };

  struct S {
    operator T<0>() = delete; // since-cxx20-note {{'operator T' has been explicitly marked deleted here}}
  };

  T<0> t(S{}); // since-cxx20-error {{call to deleted constructor of 'T<0>'}}
} // namespace DeletedConversion

namespace InaccessibleConversion {
  template <int i>
  struct T {
    T(const T&) requires(i != 0);
  };

  struct S {
  private:
    operator T<0>(); // since-cxx20-note {{declared private here}}
  };

  T<0> t(S{}); // since-cxx20-error {{'operator T' is a private member of 'cwg2327::InaccessibleConversion::S'}}
} // namespace InaccessibleConversion
#endif // __cplusplus >= 202002L

} // namespace cwg2327
