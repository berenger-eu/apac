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
  bool TraverseFunctionDecl(FunctionDecl *f) {

    if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
      return true;
    }
    if (!(f->getNameAsString().find("_apacSeq") != std::string::npos)) {
      functionsToModify.push_back(f);
    }

    return true;
  }
  inline std::vector<FunctionDecl *> getFunctionsToModify() {
    return functionsToModify;
  }

private:
  std::vector<FunctionDecl *> functionsToModify;
  Rewriter &TheRewriter;
};
