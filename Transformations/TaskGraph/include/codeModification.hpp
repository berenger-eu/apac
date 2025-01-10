#pragma once
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <sstream>
#include <string>
#include <vector>

#include "common.hpp"

void addInitApacPart(Rewriter &TheRewriter, const SourceLocation &,
                     FunctionDecl *);

void addFunctionDepth(Rewriter &TheRewriter,
                      std::vector<FunctionDecl *> &functions);
void handleTaskGroups(Rewriter &TheRewriter,
                      std::vector<FunctionDecl *> &functionStmts,
                      std::vector<ReturnStmt *> &returnStmts);
inline void modifyCode(Rewriter &TheRewriter, SourceLocation &beginCodeLoc,
                       std::vector<FunctionDecl *> &functions,
                       std::vector<ReturnStmt *> &returnStmts) {
  FunctionDecl *beginCodeFunction = nullptr;
  if (!functions.empty())
    beginCodeFunction = functions.front();
  addInitApacPart(TheRewriter, beginCodeLoc, beginCodeFunction);
  addFunctionDepth(TheRewriter, functions);
}
