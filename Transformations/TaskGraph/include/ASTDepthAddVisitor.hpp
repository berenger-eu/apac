#pragma once
#include <queue>
#include <sstream>
#include <string>

#include "common.hpp"
#include "transfoCommon.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;
class ASTDepthAddVisitor : public RecursiveASTVisitor<ASTDepthAddVisitor> {
public:
  ASTDepthAddVisitor(Rewriter &R, std::string &mainRef,
                     std::vector<std::string> &functionsRef,
                     std::vector<std::string> &functionsToIgnoreRef)
      : TheRewriter(R), mainName(mainRef), functions(functionsRef),
        functionsToIgnore(functionsToIgnoreRef) {};
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
    if (isToParseFunction(f->getNameAsString(), functions, functionsToIgnore,
                          mainName)) {
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
  std::string &mainName;
  std::vector<std::string> &functions;
  std::vector<std::string> &functionsToIgnore;
};
