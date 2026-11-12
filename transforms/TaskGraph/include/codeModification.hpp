#pragma once
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <sstream>
#include <map>
#include <string>
#include <vector>

#include "common.hpp"
class StaticCostModel;

void addInitApacPart(Rewriter &TheRewriter, const SourceLocation &,
                     FunctionDecl *, const StaticCostModel * = nullptr);
void addFunctionDepth(Rewriter &TheRewriter, std::stringstream &SSprintBefore,
                      std::stringstream &SSprintAfter, FunctionDecl *f,
                      const StaticCostModel * = nullptr);
void handleFunctionTaskGroup(Rewriter &TheRewriter, FunctionDecl *f,
                             ReturnStmt *returnStmt,
                             std::stringstream &SSprintBefore,
                             std::stringstream &SSprintAfter);
void handleFunctions(
    Rewriter &TheRewriter, std::vector<FunctionDecl *> &functions,
    std::vector<std::pair<ReturnStmt *, FunctionDecl *>> &returnStmts,
    const std::map<FunctionDecl *, std::string> &phaseTokens, const StaticCostModel *);
inline void
modifyCode(Rewriter &TheRewriter, SourceLocation &beginCodeLoc,
           std::vector<FunctionDecl *> &functions,
           std::vector<std::pair<ReturnStmt *, FunctionDecl *>> &returnStmts,
           const std::map<FunctionDecl *, std::string> &phaseTokens,
           const StaticCostModel *costs = nullptr) {
  FunctionDecl *beginCodeFunction = nullptr;
  if (!functions.empty())
    beginCodeFunction = functions.front();
  addInitApacPart(TheRewriter, beginCodeLoc, beginCodeFunction, costs);
  handleFunctions(TheRewriter, functions, returnStmts, phaseTokens, costs);
}
