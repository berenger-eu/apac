#include "core.hpp"
bool isArrayVariable(clang::VarDecl& );
bool foundCorrectFunction(clang::Decl&);
bool foundCorrectVarType(clang::VarDecl&);
bool foundCorrectVariable(clang::VarDecl&);
bool isConstantInit(clang::VarDecl& );
bool isInitNew(clang::VarDecl& );
clang::QualType unreferenceQType(clang::QualType,const clang::ASTContext& );
clang::QualType referenceToQType(clang::QualType,const clang::ASTContext&);

