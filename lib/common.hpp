#pragma once

#include <sstream>
#include <string>
#include <queue>
#include  <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;

//Contains functions used by multiple transformations

//Returns the given Stmt as a String
std::string getStmtAsString(const Stmt*,const LangOptions&);
//Returns the given Expr as a String
std::string getExprAsString(const Expr*,const LangOptions&);


//From a VarDecl Type, get the string corresponding to its declaration in a single instruction
//Useful to create multiple single declarations from a single multiple declaration
//Format: "type varName [= initValue];\n"
std::string getCompleteVarDeclStr(const VarDecl&);

//From a VarDecl Type, get the string : 
// <varName> [= <init>] ;
std::string getVarDeclDefStr(const VarDecl& );
//From a VarDecl Type, get the string :
// <type> <varName> ;
std::string getVarDeclDeclStr(const VarDecl& v);

//True when the input is a pointer type
bool isPointerQualType(clang::QualType );

//True when the input is a reference
bool isReferenceQualType(clang::QualType );

//Returns the leafs (CallExpr,DeclRefExpr,...) of a given statement
void getLeafs( clang::Stmt* s,std::vector< clang::Stmt*>& leafs);

//Returns the DeclRefExpr (single) of a given Expr, none if there are multiple
const DeclRefExpr* getSingleDeclRefExprInsideExpr(const Expr* e);
//Returns all DeclRefExpr inside a given Expr
std::vector<const DeclRefExpr*> getAllDeclRefExprInsideExpr(const Expr* e);

//True when the input is a completely constant type (exemple, const int *const )
bool isFullConstType(const QualType& qType);



//Get the depth of the pointer access 
//(-1 when getting the addres of v,0 when getting v, 1 when getting *v, 2 when getting **v, ...)
int getPtrDepthAccess(const clang::VarDecl& v,const clang::Expr& e);
int getPtrDepthAccess(QualType qt1, QualType qt2,const ASTContext& aContext);

//Returns the ArraySubscriptExpr expressions in a given Expr in the same ordre they appear
//Warning: it won't return arrays access inside indexes, so p[pi[4]] will not return pi[4]
std::deque<clang::ArraySubscriptExpr*> getArraySubscripts(const clang::Expr* e);

//Returns the values of the ArraySubscriptExpr expressions in a given Expr in the same ordre they appear
//Warning: it won't return indexes of arrays inside indexes, so p[pi[4]] will only return pi[4]

std::vector<clang::Expr*> getArraySubscriptsIndexes(const clang::Expr* e);
//Returns the values of the ArraySubscriptExpr expressions in a given Expr in the same ordre they appear
//-1 if the value is not evaluated (because it's either not evaluable or not supported)
//Warning: it won't return arrays access inside indexes, so p[pi[4]] will only return -1

std::vector<int> getArraySubscriptsIndexesValues(const clang::Expr* e);
//Inline Function 

//True if v is an Array, false otherwise
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

inline QualType getPointerToQType(QualType qt,const ASTContext& aContext){
  return aContext.getPointerType(qt);
}

//Returns the initialization part of a variable declaration as a string
inline std::string getInitString(const VarDecl& v)
{
    return getExprAsString(v.getInit(),v.getASTContext().getLangOpts());
}

inline bool isInHeaders(SourceManager& sm,SourceLocation sl)
{
  return (!(sm.isWrittenInMainFile(sl)))||sm.isInSystemHeader(sl);
}