//===- SpliceSpecifier.h - Core classes for C++ templates -------*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file provides classes for representing splice-specifiers and
//  splice-specialization-specifiers (C++26).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_SPLICESPECIFIER_H
#define LLVM_CLANG_AST_SPLICESPECIFIER_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/PointerUnion.h"
#include <optional>

namespace clang {

class ASTContext;
struct ASTTemplateArgumentListInfo;
class Expr;


/// Represents a C++26 'splice-specifier'.
class alignas(void *) SpliceSpecifier {
  SourceLocation LSpliceLoc;
  Expr *Operand;
  SourceLocation RSpliceLoc;

  SpliceSpecifier(SourceLocation LSpliceLoc, Expr *Operand,
                  SourceLocation RSpliceLoc);

public:
  void *operator new(size_t bytes, const ASTContext &C,
                     unsigned alignment = 8);

  void *operator new(size_t bytes, const ASTContext *C,
                     unsigned alignment = 8) {
    return operator new(bytes, *C, alignment);
  }

public:
  static SpliceSpecifier *Create(ASTContext &C, SourceLocation LSpliceLoc,
                                 Expr *Operand, SourceLocation RSpliceLoc);

  Expr *getOperand() const { return Operand; }
  void setOperand(Expr *E) { Operand = E; }

  bool isDependent() const;

  SourceLocation getLSpliceLoc() const { return LSpliceLoc; }
  void setLSpliceLoc(SourceLocation Loc) { LSpliceLoc = Loc; }

  SourceLocation getRSpliceLoc() const { return RSpliceLoc; }
  void setRSpliceLoc(SourceLocation Loc) { RSpliceLoc = Loc; }

  SourceLocation getBeginLoc() const { return LSpliceLoc; }

  SourceLocation getEndLoc() const { return RSpliceLoc; }

  SourceRange getSourceRange() const {
    return SourceRange(getBeginLoc(), getEndLoc());
  }
};

/// Represents a C++26 'splice-specialization-specifier'.
class alignas(void *) SpliceSpecializationSpecifier {
  SpliceSpecifier *Splice;
  const ASTTemplateArgumentListInfo *TemplateArgs;

  SpliceSpecializationSpecifier(SpliceSpecifier *Splice,
                                const ASTTemplateArgumentListInfo *Args);

public:
  void *operator new(size_t bytes, const ASTContext &C,
                     unsigned alignment = 8);

  void *operator new(size_t bytes, const ASTContext *C,
                     unsigned alignment = 8) {
    return operator new(bytes, *C, alignment);
  }

public:
  static SpliceSpecializationSpecifier *Create(
      ASTContext &C, SpliceSpecifier *Splice,
      const ASTTemplateArgumentListInfo &TemplateArgs);

  SpliceSpecifier *getSpliceSpecifier() const { return Splice; }
  void setSpliceSpecifier(SpliceSpecifier *S) { Splice = S; }

  const ASTTemplateArgumentListInfo *getTemplateArgs() const {
    return TemplateArgs;
  }

  bool isDependent() const { return Splice->isDependent(); }

  SourceLocation getLAngleLoc() const;
  SourceLocation getRAngleLoc() const;

  SourceLocation getBeginLoc() const { return Splice->getBeginLoc(); }

  SourceLocation getEndLoc() const;

  SourceRange getSourceRange() const {
    return SourceRange(getBeginLoc(), getEndLoc());
  }
};

using MaybeSpecializedSplicePtr =
    llvm::PointerUnion<SpliceSpecifier *,
                       SpliceSpecializationSpecifier *>;

} // namespace clang

#endif // LLVM_CLANG_AST_TEMPLATEBASE_H
