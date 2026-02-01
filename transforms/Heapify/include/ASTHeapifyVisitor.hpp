#pragma once
#include "utilitaryFunctions.hpp"
using namespace clang;
class ASTHeapifyVisitor : public APACRecursiveASTVisitor<ASTHeapifyVisitor> {
public:
  ASTHeapifyVisitor(Rewriter &R, struct item_found &funHeap,
                    struct item_found &varHeap, std::string &mainRef,
                    std::vector<std::string> &functionsRef,
                    std::vector<std::string> &functionsToIgnoreRef)
      : APACRecursiveASTVisitor(R, mainRef, functionsRef, functionsToIgnoreRef),
        functionHeap(funHeap), variableHeap(varHeap){};
  bool TraverseFunctionDecl(FunctionDecl *);
  bool TraverseCompoundStmt(CompoundStmt *coSt);
  inline bool VisitGotoStmt(GotoStmt *gSt) {
    gotoReturnHandle(gSt);
    return true;
  }
  inline bool VisitReturnStmt(ReturnStmt *rSt) {
    gotoReturnHandle(rSt);
    return true;
  }
  inline void gotoReturnHandle(Stmt *st) {
    if (scopeStack.top()->hasReturnGoto)
      return;
    scopeStack.top()->hasReturnGoto = true;
    scopeStack.top()->goToReturnStmts.push_back(st);
    scopeStack.top()->variablesToDelete =
        scopeStack.top()->getVariablesToDelete();
  }
  bool VisitDeclStmt(DeclStmt *st);

  inline const std::vector<std::shared_ptr<ScopeInfo>> &getTopScopes() {
    return topScopes;
  }
  inline const std::unordered_map<CompoundStmt *, std::shared_ptr<ScopeInfo>> &
  getScopes() {
    return scopes;
  }

private:
  std::unordered_map<CompoundStmt *, std::shared_ptr<ScopeInfo>> scopes;
  std::stack<std::shared_ptr<ScopeInfo>> scopeStack;
  std::vector<std::shared_ptr<ScopeInfo>> topScopes;
  struct item_found functionHeap;
  struct item_found variableHeap;
};