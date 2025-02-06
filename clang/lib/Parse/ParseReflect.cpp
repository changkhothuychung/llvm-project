//===--- ParseReflect.cpp - C++2c Reflection Parsing (P2996) --------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements parsing for reflection facilities.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/DiagnosticParse.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
using namespace clang;

ExprResult Parser::ParseCXXReflectExpression(SourceLocation OpLoc) {
  SourceLocation OperandLoc = Tok.getLocation();

  Sema::ConstevalOnlyRecorder RecordConstevalOnly(Actions);
  EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::ReflectionContext);

  // Parse a leading nested-name-specifier, e.g.,
  //
  CXXScopeSpec SS;
  if (ParseOptionalCXXScopeSpecifier(SS, /*ObjectType=*/nullptr,
                                     /*ObjectHasErrors=*/false,
                                     /*EnteringContext=*/false)) {
    SkipUntil(tok::semi, StopAtSemi | StopBeforeMatch);
    return ExprError();
  }

  // Start the tentative parse: This will be reverted if the operand is found
  // to be a type (or rather: a type whose name is more complicated than a
  // single identifier).
  //
  TentativeParsingAction TentativeAction(*this);

  // Next, check for an unqualified-id.
  if (Tok.isOneOf(tok::identifier, tok::kw_operator, tok::kw_template,
                  tok::tilde, tok::annot_template_id)) {
    // Try parsing the operand name as an 'unqualified-id'.

    SourceLocation TemplateKWLoc;
    UnqualifiedId UnqualName;
    if (!ParseUnqualifiedId(SS, ParsedType{}, /*ObjectHadError=*/false,
                            /*EnteringContext=*/false,
                            /*AllowDestructorName=*/true,
                            /*AllowConstructorName=*/false,
                            /*AllowDeductionGuide=*/false,
                            SS.isSet() ? &TemplateKWLoc : nullptr,
                            UnqualName)) {
      bool AssumeType = false;
      if (UnqualName.getKind() == UnqualifiedIdKind::IK_TemplateId &&
          UnqualName.TemplateId->Kind == TNK_Type_template)
        AssumeType = true;
      else if (Tok.isOneOf(tok::l_square, tok::l_paren, tok::star, tok::amp,
                           tok::ampamp, tok::kw_const, tok::kw_volatile,
                           tok::kw_restrict))
        AssumeType = true;

      if (!AssumeType) {
        TentativeAction.Commit();
        return RecordConstevalOnly.RecordAndReturn(
                Actions.ActOnCXXReflectExpr(OpLoc, TemplateKWLoc, SS,
                                            UnqualName));
      }
    }
  } else if (SS.isValid() &&
             SS.getScopeRep()->getKind() == NestedNameSpecifier::Global) {
    // Check for '^::'.
    TentativeAction.Commit();

    Decl *TUDecl = Actions.getASTContext().getTranslationUnitDecl();
    return RecordConstevalOnly.RecordAndReturn(
            Actions.ActOnCXXReflectExpr(OpLoc, SourceLocation(), TUDecl));
  }
  TentativeAction.Revert();

  if (SS.isSet() &&
      TryAnnotateTypeOrScopeTokenAfterScopeSpec(SS, true,
                                                ImplicitTypenameContext::No)) {
    SkipUntil(tok::semi, StopAtSemi | StopBeforeMatch);
    return ExprError();
  }

  // Anything else must be a type-id (e.g., 'const int', 'Cls(*)(int)'.
  if (isCXXTypeId(TypeIdAsReflectionOperand)) {
    TypeResult TR = ParseTypeName(nullptr, DeclaratorContext::ReflectOperator);
    if (TR.isInvalid())
      return ExprError();

    return RecordConstevalOnly.RecordAndReturn(
            Actions.ActOnCXXReflectExpr(OpLoc, TR));
  }

  Diag(OperandLoc, diag::err_cannot_reflect_operand);
  return ExprError();
}

ExprResult Parser::ParseCXXMetafunctionExpression() {
  assert(Tok.is(tok::kw___metafunction) && "expected '___metafunction'");
  SourceLocation KwLoc = ConsumeToken();

  // Balance any number of arguments in parens.
  BalancedDelimiterTracker Parens(*this, tok::l_paren);
  if (Parens.expectAndConsume())
    return ExprError();

  SmallVector<Expr *, 2> Args;
  do {
    ExprResult Expr = ParseConstantExpression();
    if (Expr.isInvalid()) {
      Parens.skipToEnd();
      return ExprError();
    }
    Args.push_back(Expr.get());
  } while (TryConsumeToken(tok::comma));

  if (Parens.consumeClose())
    return ExprError();

  SourceLocation LPLoc = Parens.getOpenLocation();
  SourceLocation RPLoc = Parens.getCloseLocation();
  return Actions.ActOnCXXMetafunction(KwLoc, LPLoc, Args, RPLoc);
}

bool Parser::ParseSpliceSpecifier() {
  assert(Tok.is(tok::l_splice) && "expected '[:'");

  BalancedDelimiterTracker SpliceTokens(*this, tok::l_splice);
  if (SpliceTokens.expectAndConsume())
    return true;

  ExprResult ER = ParseConstantExpression();
  if (ER.isInvalid() || ER.get()->containsErrors()) {
    SpliceTokens.skipToEnd();
    return true;
  }
  Expr *Operand = ER.get();

  Token end = Tok;
  if (SpliceTokens.consumeClose())
    return true;

  SourceLocation LSplice = SpliceTokens.getOpenLocation();
  SourceLocation RSplice = SpliceTokens.getCloseLocation();

  SpliceResult SR = Actions.ActOnSpliceSpecifier(LSplice, Operand, RSplice);
  if (SR.isInvalid())
    return true;
  SpliceSpecifier *Splice = SR.get();

  UnconsumeToken(end);
  Tok.setKind(tok::annot_splice);
  setSpliceAnnotation(Tok, Splice);
  Tok.setLocation(LSplice);
  Tok.setAnnotationEndLoc(RSplice);
  PP.AnnotateCachedTokens(Tok);

  return false;
}

bool Parser::ParseSpliceSpecializationSpecifier() {
  assert(Tok.isOneOf(tok::l_splice, tok::annot_splice) && "expected '[:'");

  if (Tok.is(tok::l_splice))
    if (ParseSpliceSpecifier())
      return true;

  SpliceResult SR = getSpliceAnnotation(Tok);
  if (SR.isInvalid() || !NextToken().is(tok::less))
    return true;
  ConsumeAnnotationToken();

  ASTTemplateArgsPtr TArgsPtr;
  SourceLocation LAngleLoc, RAngleLoc;
  {
    TemplateArgList TArgs;
    if (ParseTemplateIdAfterTemplateName(/*ConsumeLastToken=*/false,
                                         LAngleLoc, TArgs, RAngleLoc,
                                         /*Template=*/nullptr))
      return true;

    TArgsPtr = ASTTemplateArgsPtr(TArgs.data(), TArgs.size());
  }
  SpliceSpecResult SSR =
      Actions.ActOnSpliceSpecializationSpecifier(SR.get(), LAngleLoc, TArgsPtr,
                                                 RAngleLoc);
  if (SSR.isInvalid())
    return true;
  SpliceSpecializationSpecifier *SSS = SSR.get();

  Tok.setKind(tok::annot_splice_specialization);
  setSpliceSpecializationAnnotation(Tok, SSS);
  Tok.setLocation(SR.get()->getBeginLoc());
  Tok.setAnnotationEndLoc(SSS->getEndLoc());

  return false;
}

ExprResult Parser::ParseCXXSpliceAsExpr(SourceLocation TemplateKWLoc,
                                        bool AllowMemberReference) {
  assert(Tok.isOneOf(tok::annot_splice, tok::annot_splice_specialization) &&
         "expected a splice annotation");

  MaybeSpecializedSplicePtr Splice;
  if (Tok.is(tok::annot_splice_specialization)) {
    assert(TemplateKWLoc.isValid());

    SpliceSpecResult SSR = getSpliceSpecializationAnnotation(Tok);
    if (SSR.isInvalid())
      return ExprError();
    Splice = SSR.get();
  } else {
    SpliceResult SR = getSpliceAnnotation(Tok);
    if (SR.isInvalid())
      return ExprError();
    Splice = SR.get();
  }
  ConsumeAnnotationToken();

  return Actions.ActOnCXXSpliceExpression(TemplateKWLoc, Splice,
                                          AllowMemberReference);
}

TypeResult Parser::ParseCXXSpliceAsType(SourceLocation TypenameKWLoc,
                                        bool AllowDependent, bool Complain) {
  assert(Tok.isOneOf(tok::annot_splice, tok::annot_splice_specialization) &&
         "expected a splice annotation");

  MaybeSpecializedSplicePtr Splice;
  if (Tok.is(tok::annot_splice_specialization)) {
    SpliceSpecResult SSR = getSpliceSpecializationAnnotation(Tok);
    if (SSR.isInvalid() || (!AllowDependent && SSR.get()->isDependent()))
      return TypeError();
    Splice = SSR.get();
  } else {
    SpliceResult SR = getSpliceAnnotation(Tok);
    if (SR.isInvalid() || (!AllowDependent && SR.get()->isDependent()))
      return TypeError();
    Splice = SR.get();
  }

  TypeResult Result = Actions.ActOnCXXSpliceTypeSpecifier(TypenameKWLoc, Splice,
                                                          Complain);
  if (!Result.isInvalid())
    ConsumeAnnotationToken();

  return Result;
}

DeclResult Parser::ParseCXXSpliceAsNamespace() {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  Token SpliceTok = Tok;
  ConsumeAnnotationToken();

  SpliceResult SR = getSpliceAnnotation(SpliceTok);
  assert(!SR.isInvalid());

  return Actions.ActOnCXXSpliceExpectingNamespace(SR.get());
}
