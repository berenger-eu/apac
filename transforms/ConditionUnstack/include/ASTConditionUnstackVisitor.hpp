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
    : public APACRecursiveASTVisitor<ASTConditionUnstackVisitor> {
public:
  ASTConditionUnstackVisitor(Rewriter &R, std::string &mainRef,
                             std::vector<std::string> &functionsRef,
                             std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef,
                                functionsToIgnoreRef) {}
  bool VisitWhileStmt(WhileStmt *whileSt);
  bool VisitIfStmt(IfStmt *ifSt);
  bool VisitForStmt(ForStmt *forSt);
};