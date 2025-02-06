#pragma once
#include "helperFunctions.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <queue>
#include <sstream>
#include <stack>
#include <string>

#include "common.hpp"

using namespace clang;
class ASTUnstackVisitor : public RecursiveASTVisitor<ASTUnstackVisitor> {
public:
  ASTUnstackVisitor(Rewriter &R)
      : TheRewriter(R), tempVarsCounter(0), callsToIgnore(0) {};
  inline bool VisitStmt(Stmt *) { return true; }
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if ((!isInHeaders(TheRewriter.getSourceMgr(), fDecl->getBeginLoc())) &&
        fDecl->getNameAsString().find("_apacSeq") == std::string::npos) {
      tempVarsCounter = 0;
      callsToUnstack.push_back({});
      return RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  inline bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *fDecl) {
    if ((!isInHeaders(TheRewriter.getSourceMgr(), fDecl->getBeginLoc())) &&
        fDecl->getNameAsString().find("invalid_ref") == std::string::npos) {
      return RecursiveASTVisitor::TraverseFunctionTemplateDecl(fDecl);
    }
    return true;
  }

  inline bool TraverseDeclStmt(DeclStmt *declSt) {
    if (!isInHeaders(TheRewriter.getSourceMgr(), declSt->getBeginLoc())) {
      retrieveCalls(declSt);
    }
    return true;
  }
  inline bool TraverseCallExpr(CallExpr *calExpr) {
    if (!isInHeaders(TheRewriter.getSourceMgr(), calExpr->getBeginLoc())) {
      retrieveCalls(calExpr);
    }
    return true;
  }
  inline bool TraverseBinaryOperator(BinaryOperator *bop) {
    if (!isInHeaders(TheRewriter.getSourceMgr(), bop->getBeginLoc())) {
      retrieveCalls(bop);
    }
    return true;
  }
  inline bool TraverseUnaryOperator(UnaryOperator *uop) {
    if (!isInHeaders(TheRewriter.getSourceMgr(), uop->getBeginLoc())) {
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

  Rewriter &TheRewriter;
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
