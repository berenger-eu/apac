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
/*
// Adds a delete section in a generated scope (used when a section had to be
// created to handle variable declarations in If statement or similar types of
// statements)
void ASTHeapifyVisitor::deleteSectionAfterCreatedScope(
    const SourceLocation &deleteLoc,
    const std::vector<struct item_found> &currentVarsInScope) {

      std::stringstream SSprint;
  SSprint << ";\n" << createDeleteSegment(currentVarsInScope) << "}";
  TheRewriter.InsertTextAfterToken(deleteLoc, SSprint.str());
  for (long unsigned int i = 0; i < currentVarsInScope.size(); i++) {
    currentVarsEncountered.pop_back();
  }

}
*/
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
/*
std::string ASTHeapifyVisitor::stringDeclStmt(
    DeclStmt *st, std::vector<struct item_found> &currentVarsInScope) {
  DeclStmt *decStmt = cast<DeclStmt>(st);
  DeclGroupRef decGrpRef = decStmt->getDeclGroup();
  std::stringstream SSprint;
  for (DeclGroupRef::iterator curDeclPtr = decGrpRef.begin(),
                              decGrpEnd = decGrpRef.end();
       curDeclPtr != decGrpEnd; curDeclPtr++) {
    Decl *curDecl = *curDeclPtr;
    if (curDecl != NULL && isa<VarDecl>(curDecl)) {
      SSprint << subVisitVarDecl(*(cast<VarDecl>(curDecl)), currentVarsInScope);
    }
  }
  return SSprint.str();
}

std::string ASTHeapifyVisitor::subVisitVarDecl(
    VarDecl &v, std::vector<item_found> &currentVarsInScope) {
  // True when VarDecl corresponds to the searched variable
  std::string strRes;
  /*
  if (foundCorrectVariable(v, variableHeap.name) && !isInitNew(v)) {
    struct item_found curVar;
    initItem(curVar, v);

    // We add it to the variables in the scope and the ones encountered so far
    currentVarsInScope.push_back(curVar);
    currentVarsEncountered.push_back(curVar);
    // TOTEST
    strRes = createCreationString(curVar, TheRewriter.getLangOpts());
  } else {
    strRes = getCompleteVarDeclStr(v);
  }

  return strRes;
}
bool ASTHeapifyVisitor::deleteSegmentAtStmt(Stmt &st) {
  // We have to delete the variable when it has been found (and thus created)
  TheRewriter.InsertText(st.getBeginLoc(),
                         createDeleteSegment(currentVarsEncountered));
  return true;
}
void ASTHeapifyVisitor::initItem(struct item_found &item, VarDecl &vDec) {
  item.name = vDec.getNameAsString();
  item.uid = varCounter[vDec.getNameAsString()];
  item.array = isArrayVariable(vDec);
  item.found = true;
  varCounter[vDec.getNameAsString()]++;
  item.declaration = &vDec;
  item.qTypeNew = vDec.getType();
  if (vDec.getType().getTypePtrOrNull()->isReferenceType() ||
      !isInitAReference(vDec)) {
    item.qTypeNew = getUnreferencedQType(item.qTypeNew, vDec.getASTContext());
  }
  if (item.array) {

    item.qTypeTempMem =
        vDec.getASTContext().getPointerType(vDec.getType()
                                                .getTypePtrOrNull()
                                                ->getAsArrayTypeUnsafe()
                                                ->getElementType());
    item.qTypeVar =
        getReferenceToQType(item.qTypeTempMem, vDec.getASTContext());
  } else {

    item.qTypeTempMem = vDec.getASTContext().getPointerType(item.qTypeNew);
    item.qTypeTempMem.addConst();
    item.qTypeVar = getReferenceToQType(item.qTypeNew, vDec.getASTContext());
  }
}

*/