#pragma once

#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Basic/SourceManager.h"
using namespace clang;

// Contains functions used by multiple transformations

// Returns the given Stmt as a String
std::string getStmtAsString(const Stmt *, const LangOptions &, bool = false);
// Returns the given Expr as a String
std::string getExprAsString(const Expr *, const LangOptions &, bool = false);
// Returns the given Stmt as a String, with full information (including
// children)
std::string getStmtAsStringFull(const Stmt *statement,
                                const LangOptions &langOpt);

// From a VarDecl Type, get the string corresponding to its declaration in a
// single instruction Useful to create multiple single declarations from a
// single multiple declaration Format: "type varName [= initValue];\n"
// Most likely best method to use, relies almost only on Clang
std::string getCompleteVarDeclStr(VarDecl *);
// std::string getCompleteVarDeclStr(const VarDecl &);
// std::string getCompleteVarDeclStr(const VarDecl &, const LangOptions &);

// From a VarDecl Type, get the string :
//  <varName> [= <init>];
//  or <varName>(<init>);
// To use when linked to the declaration
std::string getVarDeclDeclDefStr(VarDecl *v);

// From a VarDecl Type, get the string :
//  <varName> [= <init>] ;
// To use when separated from the declaration
std::string getVarDeclDefStr(VarDecl *);
// From a VarDecl Type, get the string :
//  <type> <varName> ;
std::string getVarDeclDeclStr(VarDecl *v);

// True when the input is a pointer type
bool isPointerQualType(clang::QualType);

// True when the input is a reference
bool isReferenceQualType(clang::QualType);
// Returns the type of the reference
QualType getNonReferenceQualType(QualType qType);

// Returns the lowest type of a list of expressions
// So, int* is int,int& is int
// TODO: MyClass would be MyClassAttribute1,MyClassAttribute2,MyClassAttribute3
void getLowestType(std::vector<Expr *> &exprs,
                   std::vector<QualType> &lowestTypes,
                   const ASTContext &aContext);
void getLowestType(std::vector<const Expr *> &exprs,
                   std::vector<QualType> &lowestTypes,
                   const ASTContext &aContext);
// Returns the leafs (CallExpr,DeclRefExpr,...) of a given statement
// TODO: Check usefulness and remove/replace if not needed
void getLeafs(clang::Stmt *s, std::vector<clang::Stmt *> &leafs);
// Returns the leafs of a given statement as elements of the template type
template <typename T>
void getLeafsOfType(clang::Stmt *s, std::vector<T *> &leafs) {
  if (!s)
    return;

  if (isa<T>(s))
    leafs.push_back(cast<T>(s));
  for (Stmt *child : s->children()) {
    getLeafsOfType(child, leafs);
  }
}
template <typename T>
void getLeafsOfType(const clang::Stmt *s, std::vector<const T *> &leafs) {
  if (!s)
    return;

  if (isa<T>(s))
    leafs.push_back(cast<T>(s));
  for (const Stmt *child : s->children()) {
    getLeafsOfType(child, leafs);
  }
}
// Returns the DeclRefExpr (single) of a given Expr, none if there are multiple
const DeclRefExpr *getSingleDeclRefExprInsideExpr(const Expr *e);
// Returns all DeclRefExpr inside a given Expr
std::vector<const DeclRefExpr *> getAllDeclRefExprInsideExpr(const Expr *e);
// Get all distinct DeclRefExpr inside a given Expr
inline std::unordered_set<const DeclRefExpr *>
getAllDistinctDeclRefExprInsideExpr(const Expr *e) {
  std::unordered_set<const DeclRefExpr *> declSet;
  for (const auto &d : getAllDeclRefExprInsideExpr(e))
    declSet.insert(d);
  return declSet;
}
// Returns the ArraySubscriptExpr inside a given Expr, none if there are
// multiple array access (except when inside indexes)
// Example : p[pi[4]] will return p, pi[4] + pi[2] will return nullptr
// p[1][4] will return p[1][4]
const ArraySubscriptExpr *getSingleArraySubscriptExprInsideExpr(const Expr *e);
// True when the input is a completely constant type (exemple, const int *const
// )
bool isFullConstType(const QualType &qType);

// Get the depth of the pointer access
//(-1 when getting the addres of v,0 when getting v, 1 when getting *v, 2 when
// getting **v, ...)
int getPtrDepthAccess(const clang::VarDecl &v, const clang::Expr &e);
// Get the depth of the pointer access
//-1 when qt2 is a pointer to qt1, 0 when qt1 is qt2, 1 when qt1 is a pointer to
// qt2, ...
int getPtrDepthAccess(QualType pointerType, QualType variableType,
                      const ASTContext &aContext);

// Inline Function

// Get the depth of the pointer access
//(-1 when getting the addres of v,0 when getting v, 1 when getting *v, 2 when
// getting **v, ...)
int getPtrDepthAccess(const clang::VarDecl &v, const clang::Expr &e);
int getPtrDepthAccess(QualType qt1, QualType qt2, const ASTContext &aContext);

// Returns the ArraySubscriptExpr expressions in a given Expr in the same ordre
// they appear Warning: it won't return arrays access inside indexes, so
// p[pi[4]] will not return pi[4]
std::deque<const clang::ArraySubscriptExpr *>
getArraySubscripts(const clang::Expr *e);

// Returns the values of the ArraySubscriptExpr expressions in a given Expr in
// the same ordre they appear Warning: it won't return indexes of arrays inside
// indexes, so p[pi[4]] will only return pi[4]

std::vector<const clang::Expr *>
getArraySubscriptsIndexes(const clang::Expr *e);

// Returns the base decl ref expr of an ArraySubscriptExpr (nullptr if not
// found), used mostly to get the base of arrays with multiple dimensions
const clang::DeclRefExpr *
getArrayBaseDeclRefExpr(const clang::ArraySubscriptExpr *ase);

// Returns the values of the ArraySubscriptExpr expressions in a given Expr in
// the same ordre they appear -1 if the value is not evaluated (because it's
// either not evaluable or not supported) Warning: it won't return arrays access
// inside indexes, so p[pi[4]] will only return -1
std::vector<int> getArraySubscriptsIndexesValues(const clang::Expr *e);
// Inline Function

// True if v is an Array, false otherwise
inline bool isArrayVariable(VarDecl &v) {
  return (v.getType().getTypePtrOrNull() != NULL &&
          v.getType().getTypePtrOrNull()->isArrayType());
}

// Remove the reference from a qualtype and returns the result
inline QualType getUnreferencedQType(QualType qt, const ASTContext &aContext) {
  // Warning: Might lead to errors
  // Although it seems to return qt when it is not a reference (which is wanted)
  return qt.getNonReferenceType();
}
// Returns a reference to a given type
inline QualType getReferenceToQType(QualType qt, const ASTContext &aContext) {
  return aContext.getLValueReferenceType(qt);
}

inline QualType getPointerToQType(QualType qt, const ASTContext &aContext) {
  return aContext.getPointerType(qt);
}

// Returns the initialization part of a variable declaration as a string
inline std::string getInitString(VarDecl *v) {
  return getExprAsString(v->getInit(), v->getASTContext().getLangOpts());
}
// True when the input location is in a system header or not in the main file
inline bool isInHeaders(SourceManager &sm, SourceLocation sl) {
  return (!(sm.isWrittenInMainFile(sl))) || sm.isInSystemHeader(sl);
}
// True when the statement is a loop
inline bool isALoop(const Stmt *s) {
  return isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<DoStmt>(s);
}