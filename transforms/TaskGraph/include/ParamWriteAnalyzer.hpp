#pragma once

#include "common.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <unordered_map>
#include <unordered_set>

using namespace clang;

// Interprocedural analysis that determines which pointer/reference parameters
// are read-only (never written)
//
// Example:
//   long reduce(int *data, int n) {   // data is only READ
//       ...  result += data[i]; ...
//   }
// Two calls to reduce() can run in parallel because data is not written
class ParamWriteAnalyzer : public RecursiveASTVisitor<ParamWriteAnalyzer> {
public:
  void analyzeTranslationUnit(ASTContext &Ctx);

  // Returns true if parameter at index paramIdx of function f is never written
  bool isParamReadOnly(const FunctionDecl *f, unsigned paramIdx) const;

  // RecursiveASTVisitor override to visit each function definition
  bool VisitFunctionDecl(FunctionDecl *f);

private:
  std::unordered_map<const FunctionDecl *, std::unordered_set<unsigned>> writtenParams_;

  ASTContext *ctx_ = nullptr;

  void analyzeFunction(const FunctionDecl *f);

  // Check if the tree contains any write to the given parameter
  // parentFunc is used to skip self-recursive calls
  bool containsWriteTo(const Stmt *s, const ParmVarDecl *param, const FunctionDecl *parentFunc) const;

  // Check if an expression refers to the given parameter
  bool refersToParam(const Expr *e, const ParmVarDecl *param) const;
};
