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
