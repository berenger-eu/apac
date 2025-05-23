#pragma once

#include <sstream>
#include <string>

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
  void subVisitReturnStmt(ReturnStmt *);
  // Looks through the body of the ForStmt
  inline void subVisitForStmt(ForStmt *forSt) {
    handleSubStmt(forSt->getBody());
  }
  // Looks through the body of the WhileStmt
  inline void subVisitWhileStmt(WhileStmt *whileSt) {
    handleSubStmt(whileSt->getBody());
  }
  // Looks through the Then and Else part of the IfStmt
  inline void subVisitIfStmt(IfStmt *ifSt) {
    handleSubStmt(ifSt->getThen());
    handleSubStmt(ifSt->getElse());
  }
  // Will continue to Visit the compoundStmt
  void handleSubStmt(Stmt *);
  // Used to give a unique number for the exit section of each function
  unsigned int functionsCounter;
};

// Returns string : __result=<returnValue>;goto __exitX;\n
std::string createGotoString(const ReturnStmt &, const Rewriter &,
                             const unsigned int &);
