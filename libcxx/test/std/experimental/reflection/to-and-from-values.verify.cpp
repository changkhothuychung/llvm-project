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

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


                             // ==================
                             // disallowed_results
                             // ==================

namespace disallowed_results {
constexpr auto v1 = std::meta::reflect_constant((const char *)"fails");
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{provided value cannot be represented}}

struct HoldsTemporary {
  const int &tmp;
};
constexpr HoldsTemporary htmp{42};
constexpr auto v2 = std::meta::reflect_constant(htmp);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{provided value cannot be represented}}

}  // namespace disallowed_results


int main() { }
