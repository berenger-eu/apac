#pragma once
#include <queue>
#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "common.hpp"

using namespace clang;
class ASTDuplicateVisitor : public RecursiveASTVisitor<ASTDuplicateVisitor> {
public:
  ASTDuplicateVisitor(Rewriter &R) : TheRewriter(R) {};
  inline bool VisitStmt(Stmt *) { return true; }
  inline bool TraverseCXXMethodDecl(CXXMethodDecl *m) {
    return TraverseFunctionDecl(m);
  }
  bool TraverseFunctionDecl(FunctionDecl *f) {
    if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
      return true;
    }
    if (f->getNameAsString() != "main") {
      functions.push_back(f);
    }
    return true;
  }
  void addDuplicateFunctions() {
    for (auto &f : functions) {
      std::stringstream SSprint;
      SSprint << f->getReturnType().getAsString() << " "
              << f->getNameInfo().getAsString() << "(";
      for (auto &param : f->parameters()) {
        SSprint << param->getType().getAsString() << " "
                << param->getNameAsString();
        if (param != f->parameters().back()) {
          SSprint << ", ";
        }
      }
      SSprint << ")";
      SSprint << getStmtAsStringFull(f->getBody(),
                                     f->getASTContext().getLangOpts());
      TheRewriter.InsertTextAfterToken(f->getEndLoc(), SSprint.str());
    }
    llvm::errs() << "Added " << functions.size() << " functions\n";
  }

private:
  std::vector<FunctionDecl *> functions;
  Rewriter &TheRewriter;
};
