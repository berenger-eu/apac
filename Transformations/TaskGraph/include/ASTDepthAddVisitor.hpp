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
class ASTDepthAddVisitor : public RecursiveASTVisitor<ASTDepthAddVisitor> {
public:
  ASTDepthAddVisitor(Rewriter &R) : TheRewriter(R) {};
  inline bool VisitStmt(Stmt *st) { return true; }
  inline bool TraverseCXXMethodDecl(CXXMethodDecl *m) {
    return TraverseFunctionDecl(m);
  }
  inline bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *fDecl) {
    if (fDecl->getNameAsString().find("invalid_ref") == std::string::npos) {
      return RecursiveASTVisitor::TraverseFunctionTemplateDecl(fDecl);
    }
    return true;
  }
  bool VisitReturnStmt(ReturnStmt *r) {
    if (isInHeaders(TheRewriter.getSourceMgr(), r->getBeginLoc())) {
      return true;
    }
    returnStmts.push_back(r);
    return true;
  }
  bool TraverseFunctionDecl(FunctionDecl *f) {

    if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
      return true;
    }
    bool result = true;
    if (!(f->getNameAsString().find("_apacSeq") != std::string::npos)) {
      functionsToModify.push_back(f);
      result = RecursiveASTVisitor::TraverseFunctionDecl(f);
    }

    return result;
  }
  inline std::vector<FunctionDecl *> getFunctionsToModify() {
    return functionsToModify;
  }
  inline std::vector<ReturnStmt *> getReturnStmts() { return returnStmts; }

private:
  std::vector<FunctionDecl *> functionsToModify;
  std::vector<ReturnStmt *> returnStmts;
  Rewriter &TheRewriter;
};
