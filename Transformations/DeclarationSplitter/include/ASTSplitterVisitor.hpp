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
class ASTSplitterVisitor : public APACRecursiveASTVisitor<ASTSplitterVisitor> {
public:
  ASTSplitterVisitor(Rewriter &R, std::string &mainName,
                     std::vector<std::string> &functionsRef,
                     std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainName, functionsRef,
                                functionsToIgnoreRef) {}
  bool VisitDeclStmt(DeclStmt *);

private:
  bool isValidSeparation(VarDecl *);
  void stringVarDecl(VarDecl *, std::stringstream &, std::stringstream &);
};
