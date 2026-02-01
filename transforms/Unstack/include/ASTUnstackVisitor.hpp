#pragma once
#include "common.hpp"
#include "helperFunctions.hpp"
#include "transfoCommon.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <queue>
#include <sstream>
#include <stack>
#include <string>

using namespace clang;
class ASTUnstackVisitor : public APACRecursiveASTVisitor<ASTUnstackVisitor> {
public:
  ASTUnstackVisitor(Rewriter &R, std::string &mainRef,
                    std::vector<std::string> &functionsRef,
                    std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef, functionsToIgnoreRef),
        tempVarsCounter(0), callsToIgnore(0){};
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (functionsConditions(fDecl)) {
      tempVarsCounter = 0;
      callsToUnstack.push_back({});
      return RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  inline bool TraverseDeclStmt(DeclStmt *declSt) {
    if (elementsConditions(declSt)) {
      retrieveCalls(declSt);
    }
    return true;
  }
  inline bool TraverseCallExpr(CallExpr *calExpr) {
    if (elementsConditions(calExpr)) {
      retrieveCalls(calExpr);
    }
    return true;
  }
  inline bool TraverseBinaryOperator(BinaryOperator *bop) {
    if (elementsConditions(bop)) {
      retrieveCalls(bop);
    }
    return true;
  }
  inline bool TraverseUnaryOperator(UnaryOperator *uop) {
    if (elementsConditions(uop)) {
      retrieveCalls(uop);
    }
    return true;
  }
  inline std::vector<std::map<SourceLocation, std::vector<CallExpr *>>> &
  getCallsToUnstack() {
    return callsToUnstack;
  }

private:
  void retrieveCalls(DeclStmt *);
  void retrieveCalls(CallExpr *);
  void retrieveCalls(Expr *);
  inline void addCallsToUnstack(const std::vector<CallExpr *> &calls,
                                const SourceLocation &loc) {
    auto &last = callsToUnstack.back();

    if (last.count(loc) == 0) {
      last[loc] = calls;
    }

    else {
      last[loc].insert(last[loc].end(), calls.begin(), calls.end());
    }
  }
  inline void addCallToUnstack(CallExpr *calExpr, const SourceLocation &loc) {
    addCallsToUnstack({calExpr}, loc);
  }

  // one element (vector) for each function, contains all of its calls
  // associated to the location where the text has to be inserted (right
  // before the call or before the declaration etc)
  std::vector<std::map<SourceLocation, std::vector<CallExpr *>>> callsToUnstack;
  // Used to name temp variables used to store the result of function calls
  int tempVarsCounter;
  // Count the number of callExpr to ignore (because they are inside a call
  // that has been parsed already)
  unsigned int callsToIgnore;
};
