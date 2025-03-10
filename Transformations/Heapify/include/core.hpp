#pragma once
#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "common.hpp"

using namespace clang;

struct item_found {
  std::string name;
  // unique, increments by 1 for each variable with the same name
  clang::QualType qTypeTempMem;
  clang::QualType qTypeNew;
  clang::QualType qTypeVar;
  bool found;
  bool array;
  clang::VarDecl *declaration;
};
struct ScopeInfo {

  bool hasReturnGoto = false;
  // -1 when not evaluated, 0 when needed,1 when not needed
  int doesNotNeedHeap = -1;
  std::vector<VarDecl *> variables;
  std::vector<std::shared_ptr<struct ScopeInfo>> subScopes;
};
extern struct item_found variableHeap;
extern struct item_found functionHeap;
extern std::unordered_map<std::string, int> varCounter;
extern std::vector<struct item_found>
    currentVarsEncountered; // TODO implement in cleaner manner
