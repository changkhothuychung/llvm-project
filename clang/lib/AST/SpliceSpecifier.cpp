//===--- SpliceSpecifier.cpp - Class for splice specifiers ------*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements the SpliceSpecifier and SpliceSpecializationSpecifier
//  classes.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/SpliceSpecifier.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"

namespace clang {

void *SpliceSpecifier::operator new(size_t bytes, const ASTContext &C,
                                    unsigned alignment) {
  return ::operator new(bytes, C, alignment);
}

SpliceSpecifier::SpliceSpecifier(SourceLocation LSplice, Expr *Operand,
                                 SourceLocation RSplice)
: LSpliceLoc(LSplice), Operand(Operand), RSpliceLoc(RSplice) {
}

SpliceSpecifier *SpliceSpecifier::Create(ASTContext &C, SourceLocation LSplice,
                                         Expr *Operand,
                                         SourceLocation RSplice) {
  return new (C) SpliceSpecifier(LSplice, Operand, RSplice);
}

bool SpliceSpecifier::isDependent() const {
  return Operand->isValueDependent();
}

void *SpliceSpecializationSpecifier::operator new(size_t bytes,
                                                  const ASTContext &C,
                                                  unsigned alignment) {
  return ::operator new(bytes, C, alignment);
}

SpliceSpecializationSpecifier::SpliceSpecializationSpecifier(
    SpliceSpecifier *Splice, const ASTTemplateArgumentListInfo *Args)
: Splice(Splice), TemplateArgs(Args) {
}

SpliceSpecializationSpecifier *
SpliceSpecializationSpecifier::Create(ASTContext &C, SpliceSpecifier *Splice,
                                      const ASTTemplateArgumentListInfo &Args) {
  return new (C) SpliceSpecializationSpecifier(Splice, &Args);
}

SourceLocation SpliceSpecializationSpecifier::getLAngleLoc() const {
  return TemplateArgs->getLAngleLoc();
}

SourceLocation SpliceSpecializationSpecifier::getRAngleLoc() const {
  return TemplateArgs->getRAngleLoc();
}

SourceLocation SpliceSpecializationSpecifier::getEndLoc() const {
  return TemplateArgs->getRAngleLoc();
}

void *SpliceTemplateArgument::operator new(size_t bytes, const ASTContext &C,
                                           unsigned alignment) {
  return ::operator new(bytes, C, alignment);
}

SpliceTemplateArgument::SpliceTemplateArgument(
    SpliceSpecifier *Splice, std::optional<unsigned> NumExpansions,
    SourceLocation EllipsisLoc)
: Splice(Splice), EllipsisLoc(EllipsisLoc), NumExpansions(NumExpansions) {
}

SpliceTemplateArgument *
SpliceTemplateArgument::Create(ASTContext &C, SpliceSpecifier *Splice,
                               std::optional<unsigned> NumExpansions,
                               SourceLocation EllipsisLoc) {
  return new (C) SpliceTemplateArgument(Splice, NumExpansions, EllipsisLoc);
}

}  // end namespace clang
