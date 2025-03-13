#pragma once

#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "common.hpp"

using namespace clang;
class ASTConditionUnstackVisitor
    : public RecursiveASTVisitor<ASTConditionUnstackVisitor> {
public:
  ASTConditionUnstackVisitor(Rewriter &R) : TheRewriter(R) {};
  inline bool VisitStmt(Stmt *) { return true; }
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (fDecl->getNameAsString().find("_apacSeq") == std::string::npos) {
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
  bool VisitWhileStmt(WhileStmt *whileSt);
  bool VisitIfStmt(IfStmt *ifSt);
  bool VisitForStmt(ForStmt *forSt);

private:
  Rewriter &TheRewriter;
};