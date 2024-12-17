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
class ASTChangeNameVisitor : public RecursiveASTVisitor<ASTChangeNameVisitor> {
public:
  ASTChangeNameVisitor(Rewriter &R)
      : currentFunctionDecl(nullptr), TheRewriter(R) {};
  inline bool VisitStmt(Stmt *) { return true; }
  inline bool TraverseCXXMethodDecl(CXXMethodDecl *m) {
    return TraverseFunctionDecl(m);
  }
  inline bool TraverseFunctionDecl(FunctionDecl *f) {
    if (!isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
      llvm::errs() << "FunctionDecl\n";
      if (f->getNameAsString() != "main") {
        currentFunctionDecl = f;
        llvm::errs() << "FunctionDecl\n";
        TheRewriter.ReplaceText(SourceRange(f->getBeginLoc(), f->getEndLoc()),
                                "int " + f->getNameAsString() + "_apacSeq");
        return RecursiveASTVisitor::TraverseFunctionDecl(f);
      }
    }
    return true;
  }
  inline bool VisitCallExpr(CallExpr *cExpr) {

    if (cExpr && cExpr->getDirectCallee() && currentFunctionDecl != nullptr) {
      std::string calleeName = cExpr->getDirectCallee()->getNameAsString();
      std::string currentFunctionName = currentFunctionDecl->getNameAsString();
      if (currentFunctionName == calleeName) {
        llvm::errs() << "CallExpr\n";
        cExpr->dump();
        TheRewriter.ReplaceText(cExpr->getBeginLoc(), calleeName.size(),
                                calleeName + "_apacSeq");
      }
    }

    return true;
  }

private:
  FunctionDecl *currentFunctionDecl;
  Rewriter &TheRewriter;
};
