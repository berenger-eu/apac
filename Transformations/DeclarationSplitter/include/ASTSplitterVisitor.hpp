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
class ASTSplitterVisitor : public RecursiveASTVisitor<ASTSplitterVisitor> {
public:
  ASTSplitterVisitor(Rewriter &R, std::string &mainName,
                     std::vector<std::string> &functionsRef,
                     std::vector<std::string> &functionsToIgnoreRef)
      : TheRewriter(R), mainName(mainName), functions(functionsRef),
        functionsToIgnore(functionsToIgnoreRef) {};
  inline bool VisitStmt(Stmt *) { return true; }
  bool VisitDeclStmt(DeclStmt *);
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

private:
  bool isValidSeparation(VarDecl *);
  void stringVarDecl(VarDecl *, std::stringstream &, std::stringstream &);
  Rewriter &TheRewriter;
  std::string &mainName;
  std::vector<std::string> &functions;
  std::vector<std::string> &functionsToIgnore;
};
