#include "../include/core.hpp"
using namespace clang;

std::unordered_map<unsigned, FileID> fileID_table;

const_arg* getHashTableValue (NamedDecl* nd)
{
    const_arg* tableValue=NULL;
    Decl* canonicalDeclAdr=NULL;
    if(nd!=NULL && (canonicalDeclAdr= getHashKey(nd))!=NULL)
    {
        tableValue=&(const_arg_table[canonicalDeclAdr]);
    }
    return tableValue;
}
//HashKey is the adress of the canonical Decl of the Declaration, as it is unique (according to https://releases.llvm.org/10.0.0/tools/clang/docs/LibASTMatchersTutorial.html)
Decl* getHashKey(NamedDecl* nd)
{
    return nd->getCanonicalDecl();
}
//Adds a dependency (right) to a value (left) in the hash table, does nothing if either is NULL
void addDependencyHashTable(const_arg* curArg,const_arg* dependency)
{
    if(curArg!=NULL&&dependency!=NULL)
    {
        curArg->dependencies.push_back(dependency);
    }
}
//To verify more clearly if a QualType is a Pointer
bool isPointerQualType(QualType qType)
{
    const Type* typeTemp;
    bool returnValue=false;
    if((typeTemp=qType.getTypePtrOrNull()))
    {
        returnValue=typeTemp->isPointerType();
    }
    return returnValue;
}
bool isReferenceQualType(QualType qType)
{
    const Type* typeTemp;
    bool returnValue=false;
    if((typeTemp=qType.getTypePtrOrNull()))
    {
        returnValue=typeTemp->isReferenceType();
    }
    return returnValue;
}


ValueDecl* getInnerPtr(Expr* expression)
{
    ValueDecl* returnValueDecl=NULL;
    int i=0;
    while(expression!=NULL&&!isa<DeclRefExpr>(expression)&&i<10)
    { 
        i++;
        if(isa<BinaryOperator>(expression))
        {
            BinaryOperator* bopExpr =cast<BinaryOperator>(expression);
            if(isPointerQualType(bopExpr->getLHS()->getType()))
                expression=bopExpr->getLHS();
            else if (isPointerQualType(bopExpr->getRHS()->getType()))
                expression=bopExpr->getRHS();
        }
        else if(isa<UnaryOperator>(expression))
            expression=cast<UnaryOperator>(expression)->getSubExpr();
        //Calls to functions break the function, and can't be linked to a variable
        else if(isa<CallExpr>(expression))
            expression=NULL;
        else if(isa<ArraySubscriptExpr>(expression))
            expression=cast<ArraySubscriptExpr>(expression)->getBase();
        else if(isa<StringLiteral>(expression))
            expression=NULL;
        else if(isa<GNUNullExpr>(expression))
            expression=NULL;    
        else
            expression=expression->IgnoreParenCasts();

    }
    returnValueDecl=getInnerDecl(expression);
    return returnValueDecl;
}

//Retrieves, if it exists, the variable inside of an expression
ValueDecl* getInnerDecl(Expr* expression)
{
    ValueDecl* innerDecl=NULL;
    
    if(expression!=NULL)
    {
        while(expression!=NULL&&!isa<DeclRefExpr>(expression))
        {
            expression=expression->IgnoreCasts();
            if(isa<UnaryOperator>(expression))
                expression=cast<UnaryOperator>(expression)->getSubExpr();
            //We can't get the variable from a call to a function
            else if(isa<CallExpr>(expression))
                expression=NULL;
            else if(isa<ArraySubscriptExpr>(expression))
                expression=cast<ArraySubscriptExpr>(expression)->getBase();

        }
        if(expression!=NULL)
            innerDecl=cast<DeclRefExpr>(expression)->getDecl();
    }
    return innerDecl;
}