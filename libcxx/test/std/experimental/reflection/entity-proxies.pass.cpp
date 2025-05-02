//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -fentity-proxy-reflection
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


constexpr auto ctx = std::meta::access_context::current();
constexpr auto unchecked = std::meta::access_context::unchecked();

struct Base {};
enum Enum { Red };

struct A {
  int m;
  struct Inner : Base { int m; };
  using Alias = std::vector<int>;
};
struct B : private A { using A::m; using A::Inner; using A::Alias; };

struct C { template <typename> struct TCls {}; struct Inner {}; int m; };
struct D : C { using C::TCls; using C::Inner; private: using C::m; };

struct E { using Enum::Red; };
namespace InnerNS { using Enum::Red; }

struct F { int m; int n; };
struct G : F { [[=32]] using F::m; public: using F::n; };
struct H : G { using G::n; };

static_assert(is_accessible(^^A::Inner, ctx));
static_assert(!is_accessible(^^A::Inner, ctx.via(^^B)));
static_assert(is_accessible(^^B::Inner, ctx));

static_assert(is_entity_proxy(^^B::Inner));
static_assert(!is_entity_proxy(^^A::Inner));

static_assert(nonstatic_data_members_of(dealias(^^B::Inner), ctx).size() == 1);
static_assert(bases_of(dealias(^^B::Inner), ctx).size() == 1);

static_assert(template_arguments_of(dealias(^^B::Alias)) ==
              std::vector {^^int, ^^std::allocator<int>});
static_assert(identifier_of(^^B::Alias) == "Alias");
static_assert(source_location_of(^^B::Alias).line() == 34);

static_assert(type_of(dealias(^^B::m)) == ^^int);
static_assert(parent_of(^^B::m) == ^^B);
static_assert(dealias(^^B::Alias) == ^^std::vector<int>);
static_assert(template_of(dealias(^^B::Alias)) == ^^std::vector);
static_assert(has_template_arguments(dealias(^^B::Alias)));
static_assert(!has_template_arguments(^^B::Alias));

static_assert(substitute(dealias(^^D::TCls), {^^D::Inner}) ==
              ^^C::TCls<C::Inner>);

static_assert(identifier_of(members_of(^^D, unchecked)[2]) == "m");
static_assert(is_private(members_of(^^D, unchecked)[2]));

static_assert(!is_class_member(^^Enum::Red));
static_assert(is_class_member(^^E::Red));
static_assert(is_entity_proxy(^^E::Red));
static_assert(dealias(^^E::Red) == ^^Enum::Red);

static_assert(!is_namespace_member(^^Enum::Red));
static_assert(is_namespace_member(^^InnerNS::Red));

static_assert(extract<int>(annotations_of(^^G::m)[0]) == 32);
static_assert(!is_access_specified(^^G::m));
static_assert(is_access_specified(^^G::n));

static_assert(dealias(^^H::n) == ^^F::n);

                              // =================
                              // dependent_proxies
                              // =================

namespace dependent_proxies {
template <typename T>
struct S : T {
  using T::fn;
  using typename T::Inner;

  static constexpr auto r = ^^S::fn;
  static constexpr auto s = ^^fn;
  static constexpr auto t = ^^typename S::Inner;
  static constexpr auto u = ^^Inner;
};

struct A { void fn(); struct Inner {}; };
static_assert(is_entity_proxy(S<A>::r));
static_assert(is_entity_proxy(S<A>::s));
static_assert(parent_of(S<A>::r) == ^^S<A>);
static_assert(S<A>::r == S<A>::s);
static_assert(dealias(S<A>::r) == ^^A::fn);
static_assert(&[:S<A>::r:] == &A::fn);

static_assert(is_entity_proxy(S<A>::t));
static_assert(is_entity_proxy(S<A>::u));
static_assert(parent_of(S<A>::t) == ^^S<A>);
static_assert(S<A>::t == S<A>::u);
static_assert(dealias(S<A>::t) == ^^A::Inner);

                                 // ===========
                                 // using_enums
                                 // ===========

namespace using_enums {
enum class Color { Red, Green, Blue };
struct S { using enum Color; };

static_assert(^^S::Red != ^^S::Green);
static_assert(^^S::Red != ^^Color::Red);
static_assert(dealias(^^S::Red) == ^^Color::Red);
static_assert(is_entity_proxy(^^S::Red));
static_assert([:^^S::Red:] == Color::Red);

}  // namespace using_enums

}  // namespace dependent_proxies

int main() { }
