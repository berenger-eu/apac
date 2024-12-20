#pragma once
#include <queue>
#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "common.hpp"

using namespace clang;
class ASTDepthAddVisitor : public RecursiveASTVisitor<ASTDepthAddVisitor> {
public:
  ASTDepthAddVisitor(Rewriter &R) : TheRewriter(R) {};
  inline bool VisitStmt(Stmt *st) { return true; }
  inline bool TraverseCXXMethodDecl(CXXMethodDecl *m) {
    return TraverseFunctionDecl(m);
  }
  bool TraverseFunctionDecl(FunctionDecl *f) {

    if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
      return true;
    }
    bool result = true;
    if (!(f->getNameAsString().find("_apacSeq") != std::string::npos)) {
      functionsToModify.push_back(f);
      auto recCalls = recursiveCallsStatements.size();
      traverseTree(f->getBody());
      if (recCalls == recursiveCallsStatements.size()) {
        functionsToModify.pop_back();
      }
    }

    return result;
  }
  inline std::vector<FunctionDecl *> getFunctionsToModify() {
    return functionsToModify;
  }
  bool TraverseDeclStmt(DeclStmt *dStmt) {
    if (isInHeaders(TheRewriter.getSourceMgr(), dStmt->getBeginLoc())) {
      return true;
    }
    if (dStmt) {
      dStmt->dump();
      for (auto child : dStmt->children()) {
        child->dump();
      }
    }
    return true;
  }
  std::vector<Stmt *> getRecursiveCallsStatements() {
    return recursiveCallsStatements;
  }

private:
  // To look through nodes for recursive calls, we need to remember the last
  // instruction To place the depth incrementation before the recursive call
  inline void traverseTree(Stmt *st) {
    if (st == nullptr)
      return;
    for (auto child : st->children())
      handleStmt(child);
  }
  // To look for recursive calls in the given statement, true when there is one
  // or more
  bool lookForRecCall(Stmt *st);
  // To handle the given statement, by default, it looks for recursive calls in
  // it and its children
  // Then adds it to the list of statements with recursive calls
  // For specific cases (ifs, fors, etc), the handling varies so that the
  // incrementation is done before the correct Statement (which can be before
  // the if, in the if, in the else, etc)
  void handleStmt(Stmt *st);
  std::vector<FunctionDecl *> functionsToModify;
  std::vector<Stmt *> recursiveCallsStatements;
  Rewriter &TheRewriter;
};
