#include "../include/utilitaryFunctions.hpp"
using namespace clang;

bool isArrayVariable(VarDecl& v)
{
  return ( v.getType().getTypePtrOrNull()!=NULL && v.getType().getTypePtrOrNull()->isArrayType() );
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
//Returns a string containing the complete instruction for the declaration of a variable
//Used mostly for multiple declaration (since it is broken down in multiple instructions)

//Remove the reference from a qualtype and returns the result
QualType unreferenceQType(QualType qt,const ASTContext& aContext)
{
  const Type* innerType=qt.getTypePtrOrNull();
  if(innerType&&innerType->isReferenceType())
    qt=qt.getNonReferenceType();
  return qt;
}
//Returns a reference to a given type
QualType referenceToQType(QualType qt,const ASTContext& aContext)
{
  qt=aContext.getLValueReferenceType(qt);
  return qt;
}

unsigned int getUid(VarDecl& v)
{
  return varCounter[v.getNameAsString()];
}


bool foundCorrectVariable(VarDecl& vDec)
{
  return (variableHeap.name.empty()||  //If we want our action to be on all variables
  vDec.getNameAsString().compare(variableHeap.name)==0)&&  //Or if we found the variable we're looking for
  foundCorrectVarType(vDec);  //And the type is one that can be put on the heap
}
//True when the type of the variable is a type that has to be put on heap 
//(false for references for now)
bool foundCorrectVarType(VarDecl& vDec)
{
  const Type* varType=vDec.getType().getTypePtrOrNull();
  bool result=true;
  assert(varType);
  if(varType)
  {
    result=!(varType->isPointerType()||
    (varType->isReferenceType()&&!isConstantInit(vDec)));
  }
  return result;  
}
//True if the initialization does not reference a variable,false otherwise
bool isConstantInit(VarDecl& vDec)
{
  Expr* vInit=vDec.getInit();
  bool result=false;
  if(vInit)
  {
    vInit=vInit->IgnoreCasts();
    result= !isa<DeclRefExpr>(vInit);
  }
  return result;
}

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
