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
class ASTMultipleDeclSplitter
    : public APACRecursiveASTVisitor<ASTMultipleDeclSplitter> {
public:
  ASTMultipleDeclSplitter(Rewriter &R, std::string &mainRef,
                          std::vector<std::string> &functionsRef,
                          std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef,
                                functionsToIgnoreRef) {}

  inline bool VisitDeclStmt(DeclStmt *declStmt) {
    if (!declStmt->isSingleDecl()) {
      DeclGroupRef declGroup = declStmt->getDeclGroup();
      std::stringstream SSresult;
      for (Decl *decl : declGroup) {
        if (isa<VarDecl>(decl)) {
          VarDecl *varDecl = cast<VarDecl>(decl);
          SSresult << getCompleteVarDeclStr(varDecl);
        }
      }
      TheRewriter.ReplaceText(declStmt->getSourceRange(), SSresult.str());
    }
    return true;
  }
};
