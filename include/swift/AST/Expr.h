//===--- Expr.h - Swift Language Expression ASTs ----------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the Expr class and subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_AST_EXPR_H
#define SWIFT_AST_EXPR_H

#include "swift/AST/Identifier.h"
#include "swift/AST/Type.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/ADT/NullablePtr.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

namespace llvm {
  class raw_ostream;
  template <typename PT1, typename PT2>
  class PointerUnion;
}

namespace swift {
  class ASTContext;
  class Type;
  class ValueDecl;
  class Decl;
  class TypeAliasDecl;
  
enum ExprKind {
  IntegerLiteralKind,
  DeclRefExprKind,
  OverloadSetRefExprKind,
  UnresolvedDeclRefExprKind,
  UnresolvedMemberExprKind,
  UnresolvedScopedIdentifierExprKind,
  TupleExprKind,
  UnresolvedDotExprKind,
  TupleElementExprKind,
  TupleShuffleExprKind,
  ApplyExprKind,
  SequenceExprKind,
  BraceExprKind,
  ClosureExprKind,
  AnonClosureArgExprKind,
  BinaryExprKind,
  IfExprKind
};
  
  
/// Expr - Base class for all expressions in swift.
class Expr {
  Expr(const Expr&);                 // DO NOT IMPLEMENT
  void operator=(const Expr&);       // DO NOT IMPLEMENT
public:
  /// Kind - The subclass of Expr that this is.
  const ExprKind Kind;

  /// Ty - This is the type of the expression.
  Type Ty;
  
  Expr(ExprKind kind, Type ty = Type()) : Kind(kind), Ty(ty) {}

  /// getLocStart - Return the location of the start of the expression.
  /// FIXME: QOI: Need to extend this to do full source ranges like Clang.
  SMLoc getLocStart() const;
    
  enum WalkOrder {
    Walk_PreOrder,
    Walk_PostOrder
  };
  
  /// WalkExpr - This function walks all the subexpressions under this
  /// expression and invokes the specified function pointer on them.  The
  /// function pointer is invoked both before and after the children are visted,
  /// the WalkOrder specifies at each invocation which stage it is.  If the
  /// function pointer returns a non-NULL value, then the returned expression is
  /// spliced back into the AST or returned from WalkExpr if at the top-level.
  ///
  /// If function pointer returns NULL from a pre-order invocation, then the
  /// subtree is not visited.  If the function pointer returns NULL from a
  /// post-order invocation, then the walk is terminated and WalkExpr returns
  /// NULL.
  Expr *WalkExpr(Expr *(*Fn)(Expr *E, WalkOrder Order, void *Data), void *Data);
  
  
  /// ConversionRank - This enum specifies the rank of an implicit conversion
  /// of a value from one type to another.  These are ordered from cheapest to
  /// most expensive.
  enum ConversionRank {
    /// CR_Identity - It is free to convert these two types.  For example,
    /// identical types return this, types that are just aliases of each other
    /// do as well, conversion of a scalar to a single-element tuple, etc.
    CR_Identity,
    
    /// CR_AutoClosure - Conversion of the source type to the destination type
    /// requires the introduction of a closure.  This occurs with a conversion
    /// from "()" to "()->()" type, for example.
    CR_AutoClosure,
    
    /// CR_Invalid - It isn't valid to convert these types.  For example, it
    /// isn't valid to convert a value of type "()" to "(int)".
    CR_Invalid
  };
  
  /// getRankOfConversionTo - Return the rank of a conversion from the current
  /// type to the specified type.
  ConversionRank getRankOfConversionTo(Type DestTy, ASTContext &Ctx) const;
  
  void dump() const;
  void print(raw_ostream &OS, unsigned Indent = 0) const;

  // Implement isa/cast/dyncast/etc.
  static bool classof(const Expr *) { return true; }

private:
  // Make placement new and vanilla new/delete illegal for Exprs.
  void *operator new(size_t Bytes) throw();  // DO NOT IMPLEMENT.
  void operator delete(void *Data) throw();  // DO NOT IMPLEMENT.
  void *operator new(size_t Bytes, void *Mem) throw();  // DO NOT IMPLEMENT.
public:
  // Only allow allocation of Exprs using the allocator in ASTContext
  // or by doing a placement new.
  void *operator new(size_t Bytes, ASTContext &C,
                     unsigned Alignment = 8) throw();  
};


/// IntegerLiteral - Integer literal, like '4'.
class IntegerLiteral : public Expr {
public:
  StringRef Val;  // Use StringRef instead of APInt, APInt leaks.
  SMLoc Loc;
  
  IntegerLiteral(StringRef V, SMLoc L, Type Ty)
    : Expr(IntegerLiteralKind, Ty), Val(V), Loc(L) {}
  
  uint64_t getValue() const;
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const IntegerLiteral *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == IntegerLiteralKind; }
};

/// DeclRefExpr - A reference to a value, "x".
class DeclRefExpr : public Expr {
public:
  ValueDecl *D;
  SMLoc Loc;
  
  DeclRefExpr(ValueDecl *d, SMLoc L, Type Ty = Type())
    : Expr(DeclRefExprKind, Ty), D(d), Loc(L) {}
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const DeclRefExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == DeclRefExprKind; }
};

/// OverloadSetRefExpr - A reference to an overloaded set of values with a
/// single name.
class OverloadSetRefExpr : public Expr {
public:
  ArrayRef<ValueDecl*> Decls;
  SMLoc Loc;
  
  OverloadSetRefExpr(ArrayRef<ValueDecl*> decls, SMLoc L,
                     Type Ty = Type())
  : Expr(OverloadSetRefExprKind, Ty), Decls(decls), Loc(L) {}
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const OverloadSetRefExpr *) { return true; }
  static bool classof(const Expr *E) {
    return E->Kind == OverloadSetRefExprKind;
  }
};
  
/// UnresolvedDeclRefExpr - This represents use of an undeclared identifier,
/// which may ultimately be a use of something that hasn't been defined yet, it
/// may be a use of something that got imported (which will be resolved during
/// sema), or may just be a use of an unknown identifier.
///
class UnresolvedDeclRefExpr : public Expr {
public:
  Identifier Name;
  SMLoc Loc;
  
  UnresolvedDeclRefExpr(Identifier name, SMLoc loc)
    : Expr(UnresolvedDeclRefExprKind), Name(name), Loc(loc) {
  }
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const UnresolvedDeclRefExpr *) { return true; }
  static bool classof(const Expr *E) {
    return E->Kind == UnresolvedDeclRefExprKind;
  }
};

/// UnresolvedMemberExpr - This represents ':foo', an unresolved reference to a
/// member, which is to be resolved with context sensitive type information into
/// bar::foo.  These always have dependent type.
class UnresolvedMemberExpr : public Expr {
public:
  SMLoc ColonLoc;
  SMLoc NameLoc;
  Identifier Name;
  
  UnresolvedMemberExpr(SMLoc colonLoc, SMLoc nameLoc,
                       Identifier name)
    : Expr(UnresolvedMemberExprKind),
      ColonLoc(colonLoc), NameLoc(nameLoc), Name(name) {
  }
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const UnresolvedMemberExpr *) { return true; }
  static bool classof(const Expr *E) {
    return E->Kind == UnresolvedMemberExprKind;
  }
};
  
/// UnresolvedScopedIdentifierExpr - This represents "foo::bar", an unresolved
/// reference to a type foo and a member bar within it.
class UnresolvedScopedIdentifierExpr : public Expr {
public:
  TypeAliasDecl *TypeDecl;
  SMLoc TypeDeclLoc, ColonColonLoc, NameLoc;
  Identifier Name;
  
  UnresolvedScopedIdentifierExpr(TypeAliasDecl *typeDecl,
                                 SMLoc typeDeclLoc, SMLoc colonLoc,
                                 SMLoc nameLoc, Identifier name)
  : Expr(UnresolvedScopedIdentifierExprKind), TypeDecl(typeDecl),
    TypeDeclLoc(typeDeclLoc), ColonColonLoc(colonLoc), NameLoc(nameLoc),
    Name(name) {
  }
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const UnresolvedScopedIdentifierExpr *) { return true; }
  static bool classof(const Expr *E) {
    return E->Kind == UnresolvedScopedIdentifierExprKind;
  }
};
  
/// TupleExpr - Parenthesized expressions like '(x+x)' and '(x, y, 4)'.  Tuple
/// types automatically decay if they have a single element, this means that
/// single element tuple literals, such as "(4)", will exist in the AST, but
/// have a result type that is the same as the input operand type.
///
/// When a tuple element is formed with a default value for the type, the
/// corresponding SubExpr element will be null.
class TupleExpr : public Expr {
public:
  SMLoc LParenLoc;
  /// SubExprs - Elements of these can be set to null to get the default init
  /// value for the tuple element.
  // FIXME: Switch to MutableArrayRef.
  Expr **SubExprs;
  Identifier *SubExprNames;  // Can be null if no names.
  unsigned NumSubExprs;
  SMLoc RParenLoc;
  
  /// IsGrouping - True if this is a syntactic grouping expression where the
  /// source and result types are the same.  This is only true for
  /// single-element tuples with no element name.
  bool IsGrouping;
  
  /// IsPrecededByIdentifier - True if the '(' of this tuple expression was
  /// immediately preceded by an identifier.
  bool IsPrecededByIdentifier;
  
  TupleExpr(SMLoc lparenloc, Expr **subexprs, Identifier *subexprnames,
            unsigned numsubexprs, SMLoc rparenloc, bool isGrouping,
            bool isPrecededByIdentifier, Type Ty = Type())
    : Expr(TupleExprKind, Ty), LParenLoc(lparenloc), SubExprs(subexprs),
      SubExprNames(subexprnames), NumSubExprs(numsubexprs),
      RParenLoc(rparenloc), IsGrouping(isGrouping),
      IsPrecededByIdentifier(isPrecededByIdentifier) {
    assert((!isGrouping ||
            (NumSubExprs == 1 && getElementName(0).empty() && SubExprs[0])) &&
           "Invalid grouping paren");
  }

  Identifier getElementName(unsigned i) const {
    assert(i < NumSubExprs && "Invalid element index");
    return SubExprNames ? SubExprNames[i] : Identifier();
  }
  
  /// isGroupingParen - Return true if this is a grouping parenthesis, in which
  /// the input and result types are the same.
  bool isGroupingParen() const {
    return IsGrouping;
  }
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const TupleExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == TupleExprKind; }
};

/// UnresolvedDotExpr - A field access (foo.bar) on an expression with dependent
/// type.  Before type checking, the SubExpr is null (because we don't know how
/// much is bound), and during TypeChecking SubExpr may be bound to a
/// subexpression.
class UnresolvedDotExpr : public Expr {
public:
  Expr *SubExpr;       // Can be null!
  SMLoc DotLoc;
  Identifier Name;
  SMLoc NameLoc;
  
  /// ResolvedDecl - If the name refers to any local or top-level declarations,
  /// the name binder fills them in here.
  ArrayRef<ValueDecl*> ResolvedDecls;
  
  UnresolvedDotExpr(Expr *subexpr, SMLoc dotloc, Identifier name,
                    SMLoc nameloc)
  : Expr(UnresolvedDotExprKind), SubExpr(subexpr), DotLoc(dotloc),
    Name(name), NameLoc(nameloc) {}
  
  SMLoc getLocStart() const {
    return SubExpr ? SubExpr->getLocStart() : DotLoc;
  }
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const UnresolvedDotExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == UnresolvedDotExprKind;}
};

/// TupleElementExpr - Refer to an element of a tuple, e.g. "(1,2).field0".
class TupleElementExpr : public Expr {
public:
  Expr *SubExpr;
  SMLoc DotLoc;
  unsigned FieldNo;
  SMLoc NameLoc;
  
  TupleElementExpr(Expr *subexpr, SMLoc dotloc, unsigned fieldno,
                   SMLoc nameloc, Type ty = Type())
  : Expr(TupleElementExprKind, ty), SubExpr(subexpr), DotLoc(dotloc),
    FieldNo(fieldno), NameLoc(nameloc) {}
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const TupleElementExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == TupleElementExprKind; }
};

/// TupleShuffleExpr - This represents a permutation of a tuple value to a new
/// tuple type.  The expression's type is known to be a tuple type and the
/// subexpression is known to have a tuple type as well.
class TupleShuffleExpr : public Expr {
public:
  Expr *SubExpr;
  
  /// This contains an entry for each element in the Expr type.  Each element
  /// specifies which index from the SubExpr that the destination element gets.
  /// If the element value is -1, then the destination value gets the default
  /// initializer for that tuple element value.
  ArrayRef<int> ElementMapping;
  
  TupleShuffleExpr(Expr *subExpr, ArrayRef<int> elementMapping, Type Ty)
    : Expr(TupleShuffleExprKind, Ty), SubExpr(subExpr),
      ElementMapping(elementMapping) {}
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const TupleShuffleExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == TupleShuffleExprKind; }
};
  
  
/// ApplyExpr - Application of an argument to a function, which occurs
/// syntactically through juxtaposition.  For example, f(1,2) is parsed as
/// 'f' '(1,2)' which applies a tuple to the function, producing a result.
class ApplyExpr : public Expr {
public:
  /// Fn - The function being invoked.
  Expr *Fn;
  /// Argument - The one argument being passed to it.
  Expr *Arg;
  ApplyExpr(Expr *fn, Expr *arg, Type Ty)
    : Expr(ApplyExprKind, Ty), Fn(fn), Arg(arg) {}

  // Implement isa/cast/dyncast/etc.
  static bool classof(const ApplyExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == ApplyExprKind; }
};

/// SequenceExpr - a series of expressions which should be evaluated
/// sequentially, e.g. foo()  bar().  This is like BraceExpr but doesn't have
/// semicolons, braces, or declarations and can never be empty.
class SequenceExpr : public Expr {
public:
  // FIXME: Switch to MutableArrayRef.
  Expr **Elements;
  unsigned NumElements;
  
  SequenceExpr(Expr **elements, unsigned numElements)
    : Expr(SequenceExprKind), Elements(elements), NumElements(numElements) {
  }

  // Implement isa/cast/dyncast/etc.
  static bool classof(const SequenceExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == SequenceExprKind; }
};
  
/// BraceExpr - A brace enclosed sequence of expressions, like { 4; 5 }.  If the
/// final expression is terminated with a ;, the result type of the brace expr
/// is void, otherwise it is the value of the last expression.
class BraceExpr : public Expr {
public:
  SMLoc LBLoc;
  
  typedef llvm::PointerUnion<Expr*, Decl*> ExprOrDecl;
  // FIXME: Switch to MutableArrayRef.
  ExprOrDecl *Elements;
  unsigned NumElements;
  
  /// This is true if the last expression in the brace expression is missing a
  /// semicolon after it.
  bool MissingSemi;
  SMLoc RBLoc;

  BraceExpr(SMLoc lbloc, ExprOrDecl *elements,
            unsigned numelements, bool missingsemi, SMLoc rbloc)
    : Expr(BraceExprKind), LBLoc(lbloc), Elements(elements),
      NumElements(numelements), MissingSemi(missingsemi), RBLoc(rbloc) {}

  // Implement isa/cast/dyncast/etc.
  static bool classof(const BraceExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == BraceExprKind; }
};

  
/// ClosureExpr - An expression which is implicitly created by using an
/// expression in a function context where the expression's type matches the
/// result of the function.  The Decl list indicates which decls the formal
/// arguments are bound to.
class ClosureExpr : public Expr {
public:
  Expr *Input;
  
  ClosureExpr(Expr *input, Type ResultTy)
    : Expr(ClosureExprKind, ResultTy), Input(input) {}

  /// getNumArgs - Return the number of arguments that this closure expr takes.
  unsigned getNumArgs() const;
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const ClosureExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == ClosureExprKind; }
};
  
class AnonClosureArgExpr : public Expr {
public:
  unsigned ArgNo;
  SMLoc Loc;
  
  AnonClosureArgExpr(unsigned argNo, SMLoc loc)
    : Expr(AnonClosureArgExprKind), ArgNo(argNo), Loc(loc) {}
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const AnonClosureArgExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind ==AnonClosureArgExprKind;}
};
  
/// BinaryExpr - Infix binary expressions like 'x+y'.
class BinaryExpr : public Expr {
public:
  Expr *LHS;
  Expr *Fn;
  Expr *RHS;
  
  BinaryExpr(Expr *lhs, Expr *fn, Expr *rhs, Type Ty = Type())
    : Expr(BinaryExprKind, Ty), LHS(lhs), Fn(fn), RHS(rhs) {}

  // Implement isa/cast/dyncast/etc.
  static bool classof(const BinaryExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == BinaryExprKind; }
};

/// IfExpr - if/then/else expression.  If no 'else' is specified, then the
/// ElseLoc location is not specified and the Else expression is null.  The
/// condition of the 'if' is required to have a __builtin_int1 type.
class IfExpr : public Expr {
public:
  SMLoc IfLoc;
  Expr *Cond;
  Expr *Then;
  SMLoc ElseLoc;
  Expr *Else;
  
  IfExpr(SMLoc IfLoc, Expr *cond, Expr *Then, SMLoc ElseLoc,
         Expr *Else, Type Ty = Type())
    : Expr(IfExprKind, Ty),
      IfLoc(IfLoc), Cond(cond), Then(Then), ElseLoc(ElseLoc), Else(Else) {}
  
  // Implement isa/cast/dyncast/etc.
  static bool classof(const IfExpr *) { return true; }
  static bool classof(const Expr *E) { return E->Kind == IfExprKind; }
};
  
} // end namespace swift

#endif
