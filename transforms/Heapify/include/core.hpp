#pragma once
#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "transfoCommon.hpp"

#include "common.hpp"

using namespace clang;

struct item_found {
  std::string name;
  // unique, increments by 1 for each variable
  int id;
  clang::QualType qTypeTempMem;
  clang::QualType qTypeNew;
  clang::QualType qTypeVar;
  bool found;
  bool array;
  clang::VarDecl *declaration;
};

struct ScopeInfo {
  std::shared_ptr<struct ScopeInfo> parent;
  bool hasReturnGoto = false;
  // -1 when not evaluated, 0 when needed,1 when not needed
  int doesNotNeedHeap = -1;
  std::vector<VarDecl *> variablesCurScope;

  std::vector<VarDecl *> variablesToDelete;
  std::vector<Stmt *> goToReturnStmts;
  std::vector<std::shared_ptr<struct item_found>> variablesToHeap;
  std::vector<std::shared_ptr<struct item_found>> itemsToDelete;

  std::vector<std::shared_ptr<struct ScopeInfo>> subScopes;

  std::vector<VarDecl *> getVariablesToDelete() {
    std::vector<VarDecl *> varsToDelete;
    auto curScope = this;
    do {
      for (auto &var : curScope->variablesCurScope) {
        varsToDelete.push_back(var);
      }
      curScope = curScope->parent.get();
    } while (curScope != nullptr);
    return varsToDelete;
  }
};
extern struct item_found variableHeap;
extern struct item_found functionHeap;
extern std::unordered_map<std::string, int> varCounter;
extern std::vector<struct item_found>
    currentVarsEncountered; // TODO implement in cleaner manner
