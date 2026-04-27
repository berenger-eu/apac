#pragma once

#include <sstream>
#include <string>
#include <utility>

#include "transfoCommon.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "common.hpp"

using namespace clang;
class ASTGotoVisitor : public APACRecursiveASTVisitor<ASTGotoVisitor> {
public:
  ASTGotoVisitor(Rewriter &R, std::string &mainRef,
                 std::vector<std::string> &functionsRef,
                 std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef, functionsToIgnoreRef),
        functionsCounter(0) {}
  bool VisitFunctionDecl(FunctionDecl *);

private:
  // Like Visit functions, but called by VisitCompoundStmt and not by default
  // when encountering specific nodes
  void subVisitCompoundStmt(CompoundStmt *);
  // Handles the transformation of the found ReturnStmt
  void subVisitReturnStmt(ReturnStmt *, bool forceBraces = false);
  // Looks through the body of the ForStmt
  inline void subVisitForStmt(ForStmt *forSt) {
    auto *body = forSt->getBody();
    if (isa<ReturnStmt>(body)) {
      subVisitReturnStmt(cast<ReturnStmt>(body), true);
      return;
    }
    handleSubStmt(body);
  }
  // Looks through the body of the WhileStmt
  inline void subVisitWhileStmt(WhileStmt *whileSt) {
    auto *body = whileSt->getBody();
    if (isa<ReturnStmt>(body)) {
      subVisitReturnStmt(cast<ReturnStmt>(body), true);
      return;
    }
    handleSubStmt(body);
  }
  // Looks through the Then and Else part of the IfStmt
  inline void subVisitIfStmt(IfStmt *ifSt) {
    auto *thenSt = ifSt->getThen();
    if (thenSt && isa<ReturnStmt>(thenSt)) {
      subVisitReturnStmt(cast<ReturnStmt>(thenSt), true);
    } else {
      handleSubStmt(thenSt);
    }

    auto *elseSt = ifSt->getElse();
    if (elseSt && isa<ReturnStmt>(elseSt)) {
      subVisitReturnStmt(cast<ReturnStmt>(elseSt), true);
    } else {
      handleSubStmt(elseSt);
    }
  }
  // Will continue to Visit the compoundStmt
  void handleSubStmt(Stmt *);
  // Used to give a unique number for the exit section of each function
  unsigned int functionsCounter;
  std::vector<std::pair<ReturnStmt *, bool>> returnsList;
};

// Returns string : __result=<returnValue>;goto __exitX;\n
std::string createGotoString(const ReturnStmt &, const Rewriter &,
                             const unsigned int &, bool wrapInBraces = false);
