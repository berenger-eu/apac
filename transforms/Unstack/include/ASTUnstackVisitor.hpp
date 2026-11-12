#pragma once
#include "transfoCommon.hpp"
#include "clang/AST/ASTContext.h"
#include <map>

// Normalize complete statements, never insert declarations inside an operand or
// a control header. Unsupported C++ expressions remain one full expression.
class ASTUnstackVisitor : public APACRecursiveASTVisitor<ASTUnstackVisitor> {
public:
  ASTUnstackVisitor(clang::Rewriter &R, std::string &mainRef,
                    std::vector<std::string> &functionsRef,
                    std::vector<std::string> &ignored)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef, ignored) {}
  bool TraverseFunctionDecl(clang::FunctionDecl *function);
  bool TraverseCXXMethodDecl(clang::CXXMethodDecl *function) {
    return TraverseFunctionDecl(function);
  }

private:
  unsigned counter = 0;
  std::string source;
  void statement(clang::Stmt *statement, bool inCompound = false);
  void normalize(clang::Stmt *statement, clang::Expr *expression,
                 bool inCompound);
  bool safe(clang::Expr *expression) const;
  std::string
  render(clang::Expr *expression,
         const std::map<const clang::Stmt *, std::string> &names) const;
  void extract(clang::Expr *expression, bool root,
               std::map<const clang::Stmt *, std::string> &names,
               std::string &prefix);
  std::string temporary();
};
