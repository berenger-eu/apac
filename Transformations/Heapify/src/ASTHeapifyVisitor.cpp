#include "ASTHeapifyVisitor.hpp"
using namespace clang;

bool ASTHeapifyVisitor::TraverseFunctionDecl(FunctionDecl *fDecl) {
  // In case we were trying to look for a function defined in the header
  if (isInHeaders(TheRewriter.getSourceMgr(), fDecl->getEndLoc()) ||
      !foundCorrectFunction(*fDecl, functionHeap.name)) {
    return true;
  }
  functionHeap.found = true;
  auto traverseReturnValue = RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
  return traverseReturnValue;
}

bool ASTHeapifyVisitor::TraverseCompoundStmt(CompoundStmt *coSt) {
  std::shared_ptr<ScopeInfo> topScope = std::make_shared<ScopeInfo>();
  if (scopeStack.empty())
    topScopes.push_back(topScope);
  else
    scopeStack.top()->subScopes.push_back(topScope);
  scopeStack.push(topScope);
  scopes.insert({coSt, topScope});
  auto returnValue = RecursiveASTVisitor::TraverseCompoundStmt(coSt);
  scopeStack.pop();
  return returnValue;
}

bool ASTHeapifyVisitor::VisitDeclStmt(DeclStmt *declSt) {
  DeclGroupRef decGrpRef = declSt->getDeclGroup();
  for (DeclGroupRef::iterator curDeclPtr = decGrpRef.begin(),
                              decGrpEnd = decGrpRef.end();
       curDeclPtr != decGrpEnd; curDeclPtr++) {
    Decl *curDecl = *curDeclPtr;
    if (curDecl != NULL && isa<VarDecl>(curDecl)) {
      auto varDecl = cast<VarDecl>(curDecl);
      if (foundCorrectVariable(*varDecl, variableHeap.name) &&
          !isInitNew(*varDecl))
        scopeStack.top()->variables.push_back(varDecl);
    }
  }
  return true;
}
