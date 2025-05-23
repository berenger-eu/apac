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
class ASTDuplicateVisitor
    : public APACRecursiveASTVisitor<ASTDuplicateVisitor> {
public:
  ASTDuplicateVisitor(Rewriter &R, std::string &mainRef,
                      std::vector<std::string> &functionsRef,
                      std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef,
                                functionsToIgnoreRef) {}

  inline bool VisitFunctionDecl(FunctionDecl *f) {
    if (f->getNameAsString() != mainName)
      functionsDecl.push_back(f);
    return true;
  }
  void addDuplicateFunctions() {
    for (auto &f : functionsDecl) {
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
  }

private:
  std::vector<FunctionDecl *> functionsDecl;
};
