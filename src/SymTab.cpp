#include "../include/SymTab.hpp"

using namespace clang;
//Retrieve the corresponding value to an expression inside the table
const_arg* SymTab::getInnerConstArg(Expr* expression )
{
    const_arg* resultConstArg=NULL;
    if(expression!=NULL)
    {
        if (isa<MemberExpr>(expression))
        {
            MemberExpr* tempExpr=cast<MemberExpr>(expression);
            expression=tempExpr->getBase();
        }
        if(isa<CXXThisExpr>(expression))
            resultConstArg=getHashTableValue(cast<CXXThisExpr>(expression));
        else if(isPointerQualType(expression->getType()))
            resultConstArg=getHashTableValue(getInnerPtr(expression));
        else 
            resultConstArg=getHashTableValue(getInnerDecl(expression));
    }
    return resultConstArg;
}
const_arg* SymTab::getInnerConstArg(ValueDecl* valDecl){
    return getHashTableValue(valDecl);   
}
//Returns the associated value in the table
const_arg* SymTab::getHashTableValue (NamedDecl* nd)
{
    const_arg* tableValue=NULL;
    Decl* canonicalDeclAdr=NULL;
    if(nd!=NULL && (canonicalDeclAdr= getHashKey(nd))!=NULL)
        tableValue=&(const_arg_table[canonicalDeclAdr]);
    assert(tableValue!=NULL);
    return tableValue;
}
const_arg* SymTab::getHashTableValue(CXXThisExpr* thisExpr)
{
    const_arg* tableValue=NULL;
    if(thisExpr!=NULL)
        tableValue=&(const_arg_expr_table[thisExpr]);
    assert(tableValue!=NULL);
    return tableValue;
}
//HashKey is the adress of the canonical Decl of the Declaration, as it is unique (according to https://releases.llvm.org/10.0.0/tools/clang/docs/LibASTMatchersTutorial.html)
Decl* SymTab::getHashKey(NamedDecl* nd)
{
    assert(nd->getCanonicalDecl()!=NULL);
    return nd->getCanonicalDecl();
}
//Adds a dependency (right) to a value (left) in the hash table, does nothing if either is NULL
void SymTab::addDependencyHashTable(const_arg* curArg,const_arg* dependency)
{
    if(curArg!=NULL&&dependency!=NULL)
    {
        curArg->dependencies.push_back(dependency);
    }
}