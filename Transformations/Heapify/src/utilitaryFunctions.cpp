#include "utilitaryFunctions.hpp"
using namespace clang;

// Bool functions, to evaluate complex conditions

// True when it's the function we're looking for or when we want
// to put on heap all variables (and so when we're not looking for a specific
// function)
bool foundCorrectFunction(Decl &dec, const std::string &soughtFunctionName) {
  if (isa<FunctionDecl>(dec)) {
    FunctionDecl &fDec = cast<FunctionDecl>(dec);
    // If function name is NULL, then we're trying to put on the heap all
    // variables, Otherwise, we will only traverse the Function Declaration if
    // it's the one we're looking for
    if (fDec.getNameAsString().find("_apacSeq") != std::string::npos) {
      return false;
    }
    return (soughtFunctionName.empty() ||
            (fDec.getNameAsString().compare(soughtFunctionName) == 0));
  }
  return false;
}
// True when the variable can be put on the heap (so not a pointer or a
// reference to a variable)
bool foundCorrectVarType(VarDecl &vDec) {
  const Type *varType = vDec.getType().getTypePtrOrNull();
  bool result = true;
  assert(varType);
  if (varType) {
    result = !(varType->isPointerType() ||
               (varType->isReferenceType() && isInitAReference(vDec)));
  }
  return result;
}
// True when the variable can be put on the heap AND it's either the one we're
// looking for
//  or we're looking for all variables
bool foundCorrectVariable(VarDecl &vDec,
                          const std::string &soughtVariableName) {
  return (soughtVariableName
              .empty() || // If we want our action to be on all variables
          vDec.getNameAsString().compare(soughtVariableName) ==
              0) && // Or if we found the variable we're looking for
         foundCorrectVarType(
             vDec); // And the type is one that can be put on the heap
}

// True if the initialization references a variable,false otherwise
bool isInitAReference(VarDecl &vDec) {
  Expr *vInit = vDec.getInit();
  bool result = false;
  if (vInit) {
    vInit = vInit->IgnoreCasts();
    result = isa<DeclRefExpr>(vInit);
  }
  return result;
}

bool isInitNew(VarDecl &v) {
  bool result = false;
  Expr *vInit = v.getInit();
  if (vInit) {
    result = isa<CXXNewExpr>(vInit);
  }
  return result;
}

bool computeNeededHeapScope(std::shared_ptr<ScopeInfo> scope) {
  for (auto &subScope : scope->subScopes) {
    computeNeededHeapScope(subScope);
  }
  scope->doesNotNeedHeap = scope->hasReturnGoto;
  if (scope->doesNotNeedHeap != 1) {
    bool subScopesNeedHeap = false;
    for (auto &subScope : scope->subScopes) {
      subScopesNeedHeap = subScopesNeedHeap && subScope->doesNotNeedHeap;
    }
    if (subScopesNeedHeap)
      scope->doesNotNeedHeap = 0;
  }
  return scope->doesNotNeedHeap;
}

void computeNeededHeap(
    const std::vector<std::shared_ptr<ScopeInfo>> &topScopes) {
  for (auto &scope : topScopes) {
    computeNeededHeapScope(scope);
  }
}

void computeScopeVariables(
    std::shared_ptr<ScopeInfo> scope, int id,
    std::unordered_map<VarDecl *, std::shared_ptr<item_found>> &varToItem) {
  if (scope->doesNotNeedHeap == 0) {
    for (auto &var : scope->variablesCurScope) {
      if (varToItem.count(var) == 0) {
        std::shared_ptr<item_found> item = std::make_shared<item_found>();
        initItem(*item, *var, id);
        id++;
        scope->variablesToHeap.push_back(item);
        varToItem.insert({var, item});
      }
    }
  }
  for (auto &subScope : scope->subScopes) {
    subScope->parent = scope;
    computeScopeVariables(subScope, id, varToItem);
  }
  for (auto &varDel : scope->variablesToDelete) {
    llvm::errs() << "SizeDelete" << scope->variablesToDelete.size() << "\n";
    if (varToItem.count(varDel) == 1) {
      scope->itemsToDelete.push_back(varToItem[varDel]);
    }
  }
}

void initItem(struct item_found &item, VarDecl &vDec, int &id) {
  item.name = vDec.getNameAsString();
  item.id = id;
  item.array = isArrayVariable(vDec);
  item.found = true;
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
