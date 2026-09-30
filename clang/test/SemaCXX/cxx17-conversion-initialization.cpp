// RUN: %clang_cc1 -std=c++17 -verify %s
// RUN: %clang_cc1 -std=c++20 -verify %s
// RUN: %clang_cc1 -std=c++23 -verify %s
// RUN: %clang_cc1 -std=c++2c -verify %s

namespace conversion_init {
struct A {
  A(const A &) = delete;
  A(A &&) = delete;
};
struct S {
  operator const A();
};
S s;
A a(s);
} // namespace conversion_init

namespace no_conversion_init {
struct A {
  A(const A &) = delete;
  A(A &&) = delete;
};
struct D : A {};
struct R { operator A &(); };
struct P { operator D(); };
A a1(R{}); // expected-error {{call to deleted constructor of 'A'}}
           // expected-note@-7 {{marked deleted here}}
A a2(P{}); // expected-error {{call to deleted constructor of 'A'}}
           // expected-note@-8 {{marked deleted here}}
} // namespace no_conversion_init

namespace explicit_functions {
struct T { T(int); };
struct S { explicit operator T(); };
S s;
T t1(s), t2{s};
T t3 = {s}; // expected-error {{chosen conversion function is explicit in copy-list-initialization}}
            // expected-note@-4 {{explicit conversion function declared here}}

struct U { explicit U(const U &); };
struct V { operator U(); };
V v;
U u1(v), u2{v};
U u3 = {v}; // expected-error {{chosen constructor is explicit in copy-initialization}}
            // expected-note@-5 {{explicit constructor declared here}}
} // namespace explicit_functions

namespace second_phase {
struct A { A(A &); };
struct SA { operator A(); };
A a(SA{});

#if __cplusplus >= 202002L
template <int N = 0> struct B {
  B(const B &) requires(N != 0);
};
struct S { operator B<>(); };
struct Tmpl { template <class T> operator T(); };
void f(B<>);
B<> b1(S{});
B<> b2(Tmpl{});
void g(S s) { f({s}); }

struct Ref { template <class T> operator T &(); };
B<> b3(Ref{}); // expected-error {{no matching constructor for initialization of 'B<>'}}
               // expected-note@-11 {{constraints not satisfied}}
               // expected-note@-12 {{because '0 != 0' (0 != 0) evaluated to false}}

struct D : B<> { operator B<>(); }; // expected-warning {{will never be used}}
void h(D d) {
  B<> b(d); // expected-error {{no matching constructor for initialization of 'B<>'}}
            // expected-note@-17 {{constraints not satisfied}}
            // expected-note@-18 {{because '0 != 0' (0 != 0) evaluated to false}}
}

struct S2 { operator B<>(); };
struct Amb : S, S2 {};
B<> b4(Amb{}); // expected-error {{call to constructor of 'B<>' is ambiguous}}
               // expected-note@-22 {{candidate function}}
               // expected-note@-4 {{candidate function}}
B<> b5 = {Amb{}}; // expected-error {{call to constructor of 'B<>' is ambiguous}}
                  // expected-note@-25 {{candidate function}}
                  // expected-note@-7 {{candidate function}}

struct Del { operator B<>() = delete; };
B<> b6(Del{}); // expected-error {{call to deleted constructor of 'B<>'}}
               // expected-note@-2 {{explicitly marked deleted here}}
B<> b7 = {Del{}}; // expected-error {{call to deleted constructor of 'B<>'}}
                  // expected-note@-4 {{explicitly marked deleted here}}

class Priv { operator B<>(); };
B<> b8(Priv{}); // expected-error {{'operator B' is a private member of 'second_phase::Priv'}}
                // expected-note@-2 {{implicitly declared private here}}
#endif
} // namespace second_phase
