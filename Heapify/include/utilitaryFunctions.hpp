#include "core.hpp"
using namespace clang;

//Non Inline functions

//True when the function should be traversed
//So true when it is the function passed in argument to the program or
//when there is no arguments and all functions should be traversed
bool foundCorrectFunction(Decl&);
//True when the type of the variable is a type that has to be put on heap 
//(false for references and pointers for now)
bool foundCorrectVarType(VarDecl&);
//True when the variable can be put on the heap
//So when its type is valid and it's a variable we're looking for 
bool foundCorrectVariable(VarDecl&);
//True when the initialization is not a variable
bool isInitAReference(VarDecl& );
//True when there is a new in the initialization
bool isInitNew(VarDecl& );


//Inline functions

inline bool isArrayVariable(VarDecl& v) {
    return ( v.getType().getTypePtrOrNull()!=NULL && v.getType().getTypePtrOrNull()->isArrayType() );
}

//Remove the reference from a qualtype and returns the result
inline QualType getUnreferencedQType(QualType qt,const ASTContext& aContext){
  //Warning: Might lead to errors
  //Although it seems to return qt when it is not a reference (which is wanted)
  return qt.getNonReferenceType();
}
//Returns a reference to a given type
inline QualType getReferenceToQType(QualType qt,const ASTContext& aContext){
  return aContext.getLValueReferenceType(qt);
}

inline unsigned int getUid(VarDecl& v){
    return varCounter[v.getNameAsString()];
}