#pragma once
#include <queue>
#include <sstream>
#include <string>

#include "transfoCommon.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "common.hpp"

using namespace clang;
class ASTChangeNameVisitor
    : public APACRecursiveASTVisitor<ASTChangeNameVisitor> {
public:
  ASTChangeNameVisitor(Rewriter &R, std::string &mainRef,
                       std::vector<std::string> &functionsRef,
                       std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef, functionsToIgnoreRef),
        currentFunctionDecl(nullptr) {}

  inline bool VisitFunctionDecl(FunctionDecl *f) {

      currentFunctionDecl = f;
      std::stringstream SSprint;
      SSprint << f->getReturnType().getAsString() << " "
              << f->getNameInfo().getAsString() << "_apacSeq"
              << "(";
      for (auto &param : f->parameters()) {
        SSprint << param->getType().getAsString() << " "
                << param->getNameAsString();
        if (param != f->parameters().back()) {
          SSprint << ", ";
        }
      }
      SSprint << ")";
      TheRewriter.ReplaceText(
          SourceRange(f->getTypeSpecStartLoc(), f->getTypeSpecEndLoc()),
          SSprint.str());
    
    return true;
  }
  inline bool VisitCallExpr(CallExpr *cExpr) {

    if (cExpr && cExpr->getDirectCallee() && currentFunctionDecl != nullptr) {
      std::string calleeName = cExpr->getDirectCallee()->getNameAsString();
      std::string currentFunctionName = currentFunctionDecl->getNameAsString();
      if (currentFunctionName == calleeName) {
        TheRewriter.ReplaceText(cExpr->getBeginLoc(), calleeName.size(),
                                calleeName + "_apacSeq");
      }
    }

    return true;
  }

private:
  FunctionDecl *currentFunctionDecl;
};
