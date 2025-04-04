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
    : public RecursiveASTVisitor<ASTMultipleDeclSplitter> {
public:
  ASTMultipleDeclSplitter(Rewriter &R, std::string &mainRef,
                          std::vector<std::string> &functionsRef,
                          std::vector<std::string> &functionsToIgnoreRef)
      : TheRewriter(R), main(mainRef), functions(functionsRef),
        functionsToIgnore(functionsToIgnoreRef) {};

  inline bool VisitStmt(Stmt *) { return true; }
  inline bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (isToParseFunction(fDecl->getNameAsString(), functions,
                          functionsToIgnore, main)) {
      return RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  inline bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *fDecl) {
    if (fDecl->getNameAsString().find("invalid_ref") == std::string::npos) {
      return RecursiveASTVisitor::TraverseFunctionTemplateDecl(fDecl);
    }
    return true;
  }
  inline bool VisitDeclStmt(DeclStmt *declStmt) {
    if (!declStmt->isSingleDecl()) {
      DeclGroupRef declGroup = declStmt->getDeclGroup();
      std::stringstream SSresult;
      for (Decl *decl : declGroup) {
        if (isa<VarDecl>(decl)) {
          VarDecl *varDecl = cast<VarDecl>(decl);
          llvm::errs() << "VarDecl: " << varDecl->getNameAsString() << "\n";
          llvm::errs() << getCompleteVarDeclStr(varDecl) << "\n";
          SSresult << getCompleteVarDeclStr(varDecl);
        }
      }
      llvm::errs() << SSresult.str() << "\n";
      TheRewriter.ReplaceText(declStmt->getSourceRange(), SSresult.str());
    }
    return true;
  }

private:
  Rewriter &TheRewriter;
  std::string &main;
  std::vector<std::string> &functions;
  std::vector<std::string> &functionsToIgnore;
};
