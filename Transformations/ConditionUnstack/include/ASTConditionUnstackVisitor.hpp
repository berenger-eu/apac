#pragma once

#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"

#include "common.hpp"
#include "transfoCommon.hpp"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;
class ASTConditionUnstackVisitor
    : public RecursiveASTVisitor<ASTConditionUnstackVisitor> {
public:
  ASTConditionUnstackVisitor(Rewriter &R, std::string &mainRef,
                             std::vector<std::string> &functionsRef,
                             std::vector<std::string> &functionsToIgnoreRef)
      : TheRewriter(R), mainName(mainRef), functions(functionsRef),
        functionsToIgnore(functionsToIgnoreRef) {};
  inline bool VisitStmt(Stmt *) { return true; }
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (isToParseFunction(fDecl->getNameAsString(), functions,
                          functionsToIgnore, mainName)) {
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
  std::string &mainName;
  std::vector<std::string> &functions;
  std::vector<std::string> &functionsToIgnore;
};