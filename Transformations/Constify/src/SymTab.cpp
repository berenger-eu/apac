#include "SymTab.hpp"

using namespace clang;
// Retrieve the corresponding value to an expression inside the table
const_arg *SymTab::getInnerConstArg(Expr *expression) {
  const_arg *resultConstArg = nullptr;
  if (expression != nullptr) {
    if (isa<CXXThisExpr>(expression)) {
      resultConstArg = getHashTableValue(cast<CXXThisExpr>(expression));
    } else {
      Expr *valExpr = nullptr;
      if (isPointerQualType(expression->getType())) {
        valExpr = getInnerPtr(expression);
      } else {
        valExpr = getInnerExpr(expression);
      }
      // If there is a variable (!=nullptr) AND it's not a included variable
      // (not parsed so not in the table)
      if (valExpr == nullptr)
        ;
      else if (isa<CXXThisExpr>(valExpr)) {
        resultConstArg = getHashTableValue(cast<CXXThisExpr>(valExpr));
      } else if (isa<MemberExpr>(valExpr)) {
        ValueDecl *valDecl = cast<MemberExpr>(valExpr)->getMemberDecl();
        if (valDecl != nullptr && !TheRewriter.getSourceMgr().isInSystemHeader(
                                      valDecl->getLocation())) {
          resultConstArg = getHashTableValue(valDecl);
        }
      }

      else if (isa<DeclRefExpr>(valExpr)) {
        ValueDecl *valDecl = cast<DeclRefExpr>(valExpr)->getDecl();
        if (valDecl != nullptr && !TheRewriter.getSourceMgr().isInSystemHeader(
                                      valDecl->getLocation())) {
          resultConstArg = getHashTableValue(valDecl);
        }
      }
    }
  }
  return resultConstArg;
}
const_arg *SymTab::getInnerConstArg(ValueDecl *valDecl) {
  return getHashTableValue(valDecl);
}
// Returns the associated value in the table
const_arg *SymTab::getHashTableValue(NamedDecl *nd) {
  const_arg *tableValue = nullptr;
  Decl *canonicalDeclAdr = nullptr;
  if (nd != nullptr)
    canonicalDeclAdr = getHashKey(nd);
  if (canonicalDeclAdr != nullptr) {
    tableValue = &(const_arg_table[canonicalDeclAdr]);
  }
  assert(tableValue != nullptr);
  return tableValue;
}
const_arg *SymTab::getHashTableValue(CXXThisExpr *thisExpr) {
  const_arg *tableValue = nullptr;
  if (thisExpr != nullptr) {
    tableValue = &(const_arg_expr_table[thisExpr]);
  }
  assert(tableValue != nullptr);
  return tableValue;
}
// HashKey is the adress of the canonical Decl of the Declaration, as it is
// unique (according to
// https://releases.llvm.org/10.0.0/tools/clang/docs/LibASTMatchersTutorial.html)
Decl *SymTab::getHashKey(NamedDecl *nd) {
  assert(nd->getCanonicalDecl() != nullptr);
  return nd->getCanonicalDecl();
}
// Adds a dependency (right) to a value (left) in the hash table, does nothing
// if either is nullptr
void SymTab::addDependencyHashTable(const_arg *curArg, const_arg *dependency) {
  if (curArg != nullptr && dependency != nullptr) {
    curArg->dependencies.push_back(dependency);
  }
}