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
class ASTDepthAddVisitor : public APACRecursiveASTVisitor<ASTDepthAddVisitor> {
public:
  ASTDepthAddVisitor(Rewriter &R, std::string &mainRef,
                     std::vector<std::string> &functionsRef,
                     std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef,
                                functionsToIgnoreRef) {}

  bool VisitReturnStmt(ReturnStmt *r) {
    if (!elementsConditions(r)) {
      return true;
    }
    returnStmts.push_back(r);
    return true;
  }
  inline bool VisitFunctionDecl(FunctionDecl *f) {
    functionsToModify.push_back(f);
    return true;
  }
  inline std::vector<FunctionDecl *> getFunctionsToModify() {
    return functionsToModify;
  }
  inline std::vector<ReturnStmt *> getReturnStmts() { return returnStmts; }

private:
  std::vector<FunctionDecl *> functionsToModify;
  std::vector<ReturnStmt *> returnStmts;
};
