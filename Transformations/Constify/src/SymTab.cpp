#include "SymTab.hpp"

using namespace clang;
//Retrieve the corresponding value to an expression inside the table
const_arg* SymTab::getInnerConstArg(Expr* expression )
{
    const_arg* resultConstArg=NULL;
    if(expression!=NULL)
    {
        if(isa<CXXThisExpr>(expression))
            resultConstArg=getHashTableValue(cast<CXXThisExpr>(expression));
        else{
            Expr* valExpr=NULL;    
            if(isPointerQualType(expression->getType()))
                valExpr=getInnerPtr(expression);
            else 
                valExpr=getInnerExpr(expression);
            //If there is a variable (!=NULL) AND it's not a included variable (not parsed so not in the table)
            if(valExpr==NULL)
                ;
            else if(isa<CXXThisExpr>(valExpr))
                resultConstArg=getHashTableValue(cast<CXXThisExpr>(valExpr));
            else if(isa<MemberExpr>(valExpr)){
                ValueDecl* valDecl = cast<MemberExpr>(valExpr)->getMemberDecl();
                if(valDecl!=NULL&&!TheRewriter.getSourceMgr().isInSystemHeader(valDecl->getLocation()))
                    resultConstArg=getHashTableValue(valDecl);
            }

            else if(isa<DeclRefExpr>(valExpr)){
                ValueDecl* valDecl = cast<DeclRefExpr>(valExpr)->getDecl();
                if(valDecl!=NULL&&!TheRewriter.getSourceMgr().isInSystemHeader(valDecl->getLocation()))
                    resultConstArg=getHashTableValue(valDecl);
            }
        }       
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