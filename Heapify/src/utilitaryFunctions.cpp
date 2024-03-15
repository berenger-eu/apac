#include "utilitaryFunctions.hpp"
using namespace clang;

//Bool functions, to evaluate complex conditions


bool foundCorrectFunction(Decl& dec)
{
  if(isa<FunctionDecl>(dec))
  {
    FunctionDecl& fDec=cast<FunctionDecl>(dec);
    //If function name is NULL, then we're trying to put on the heap all variables,
    //Otherwise, we will only traverse the Function Declaration if it's the one we're looking for 
    return (functionHeap.name.empty()||(fDec.getNameAsString().compare(functionHeap.name)==0));
  }
  return false;
}

bool foundCorrectVarType(VarDecl& vDec)
{
  const Type* varType=vDec.getType().getTypePtrOrNull();
  bool result=true;
  assert(varType);
  if(varType)
  {
    result=!(varType->isPointerType()||
    (varType->isReferenceType()&&isInitAReference(vDec)));
  }
  return result;  
}
bool foundCorrectVariable(VarDecl& vDec)
{
  return (variableHeap.name.empty()||  //If we want our action to be on all variables
  vDec.getNameAsString().compare(variableHeap.name)==0)&&  //Or if we found the variable we're looking for
  foundCorrectVarType(vDec);  //And the type is one that can be put on the heap
}

//True if the initialization references a variable,false otherwise
bool isInitAReference(VarDecl& vDec)
{
  Expr* vInit=vDec.getInit();
  bool result=false;
  if(vInit)
  {
    vInit=vInit->IgnoreCasts();
    result= isa<DeclRefExpr>(vInit);
  }
  return result;
}

bool isInitNew(VarDecl& v)
{
  bool result=false;
  Expr* vInit=v.getInit();
  if(vInit)
  {
    result=isa<CXXNewExpr>(vInit);
  }
  return result;
}



