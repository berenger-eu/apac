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
class ASTMainParaVisitor : public RecursiveASTVisitor<ASTMainParaVisitor> {
public:
  ASTMainParaVisitor(Rewriter &R) : TheRewriter(R) {
    mainFuncReturnStmt = nullptr;
    resultWrapperDecl = nullptr;
  };
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
  inline bool VisitReturnStmt(ReturnStmt *r) {
    if (isInHeaders(TheRewriter.getSourceMgr(), r->getBeginLoc())) {
      return true;
    }
    mainFuncReturnStmt = r;
    return true;
  }
  inline bool TraverseFunctionDecl(FunctionDecl *f) {

    if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
      return true;
    }
    bool result = true;
    if ((f->getNameAsString().find("main") != std::string::npos)) {
      result = RecursiveASTVisitor::TraverseFunctionDecl(f);
    }

    return result;
  }
  inline void addParaZone() {
    std::stringstream SSprintBefore, SSprintAfter;
    if (resultWrapperDecl == nullptr || mainFuncReturnStmt == nullptr) {
      return;
    }
    SSprintBefore << "\n#pragma omp parallel num_threads(nb_cores)\n "
                  << "#pragma omp master\n{";
    SSprintAfter << "}\n";
    TheRewriter.InsertTextAfterToken(resultWrapperDecl->getEndLoc(),
                                     SSprintBefore.str());
    TheRewriter.InsertTextBefore(mainFuncReturnStmt->getBeginLoc(),
                                 SSprintAfter.str());
  }

private:
  ReturnStmt *mainFuncReturnStmt;
  DeclStmt *resultWrapperDecl;
  Rewriter &TheRewriter;
};
