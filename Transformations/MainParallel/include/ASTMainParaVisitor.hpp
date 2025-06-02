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
class ASTMainParaVisitor : public APACRecursiveASTVisitor<ASTMainParaVisitor> {
public:
  ASTMainParaVisitor(Rewriter &R, std::string &mainName,
                     const std::vector<std::string> &functionsRef,
                     const std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainName, functionsRef,
                                functionsToIgnoreRef) {
    mainFuncReturnStmt = nullptr;
    resultWrapperDecl = nullptr;
  };
  inline bool VisitDeclStmt(DeclStmt *d) {
    if (isInHeaders(TheRewriter.getSourceMgr(), d->getBeginLoc())) {
      return true;
    }
    if (d->isSingleDecl() && isa<VarDecl>(d->getSingleDecl())) {
      VarDecl *var = cast<VarDecl>(d->getSingleDecl());
      if (var->getNameAsString().find("__result") != std::string::npos) {
        resultWrapperDecl = d;
      }
    }
    return true;
  }
  inline bool VisitIfStmt(IfStmt *ifSt) {
    if (elementsConditions(ifSt)) {
      firstValidFunctionIf = ifSt;
    }
    return true;
  }
  inline bool VisitReturnStmt(ReturnStmt *r) {
    if (isInHeaders(TheRewriter.getSourceMgr(), r->getBeginLoc())) {
      return true;
    }
    mainFuncReturnStmt = r;
    return true;
  }
  inline bool TraverseFunctionDecl(FunctionDecl *f) {
    bool result = true;
    if (functionsConditions(f) && (f->getNameAsString() == mainName)) {
      result = RecursiveASTVisitor::TraverseFunctionDecl(f);
      mainFuncDecl = f;
    }
    return result;
  }
  inline void addParaZone() {
    std::stringstream SSprintBefore, SSprintAfter;
    SourceLocation locBefore, locAfter;
    locBefore = firstValidFunctionIf->getThen()->getBeginLoc();
    locAfter = firstValidFunctionIf->getThen()->getEndLoc();
    SSprintBefore << "\n#pragma omp parallel num_threads(nb_cores)\n "
                  << "#pragma omp master\n{";
    SSprintAfter << "}\n";
    TheRewriter.InsertTextAfterToken(locBefore, SSprintBefore.str());
    TheRewriter.InsertTextBefore(locAfter, SSprintAfter.str());
  }

private:
  IfStmt *firstValidFunctionIf = nullptr;
  ReturnStmt *mainFuncReturnStmt;
  DeclStmt *resultWrapperDecl;
  FunctionDecl *mainFuncDecl = nullptr;
};
