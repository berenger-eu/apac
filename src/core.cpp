#include "../include/core.hpp"
using namespace clang;

std::unordered_map<unsigned, FileID> fileID_table;
std::unordered_map<Decl*, struct const_arg> const_arg_table;
std::unordered_map<Expr*, struct const_arg> const_arg_expr_table;
CXXMethodDecl* lastMethDecl=NULL;
const_arg* getHashTableValue (NamedDecl* nd)
{
    const_arg* tableValue=NULL;
    Decl* canonicalDeclAdr=NULL;
    if(nd!=NULL && (canonicalDeclAdr= getHashKey(nd))!=NULL)
        tableValue=&(const_arg_table[canonicalDeclAdr]);
    assert(tableValue!=NULL);
    return tableValue;
}
const_arg* getHashTableValue(CXXThisExpr* thisExpr)
{
    const_arg* tableValue=NULL;
    if(thisExpr!=NULL)
        tableValue=&(const_arg_expr_table[thisExpr]);
    assert(tableValue!=NULL);
    return tableValue;
}
//HashKey is the adress of the canonical Decl of the Declaration, as it is unique (according to https://releases.llvm.org/10.0.0/tools/clang/docs/LibASTMatchersTutorial.html)
Decl* getHashKey(NamedDecl* nd)
{
    assert(nd->getCanonicalDecl()!=NULL);
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
    int i=0;    //To avoid loops and still constify the file 
    while(expression!=NULL&&returnValueDecl==NULL&&i<10)
    { 
        i++;
        if(isa<DeclRefExpr>(expression))
        {
            returnValueDecl=getInnerDecl(expression);
            expression=NULL;
        }
        else if(isa<MemberExpr>(expression))
        {
            returnValueDecl=cast<MemberExpr>(expression)->getMemberDecl();
            expression=NULL;
        }
        else if(isa<BinaryOperator>(expression))
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
    assert(i<10);
    
    return returnValueDecl;
}



//Retrieves, if it exists, the variable inside of an expression
ValueDecl* getInnerDecl(Expr* expression)
{
    ValueDecl* innerDecl=NULL;
    
    if(expression!=NULL)
    {
        int i=0;    //To avoid loops and still constify the file 
        while(expression!=NULL&&innerDecl==NULL&&i<10)
        {
            i++;
            expression=expression->IgnoreCasts();
            if(isa<DeclRefExpr>(expression))
                innerDecl=cast<DeclRefExpr>(expression)->getDecl();
            else if(isa<MemberExpr>(expression))
                innerDecl=cast<MemberExpr>(expression)->getMemberDecl();
            else if(isa<UnaryOperator>(expression))
                expression=cast<UnaryOperator>(expression)->getSubExpr();
            //We can't get the variable from a call to a function
            else if(isa<CallExpr>(expression))
                expression=NULL;
            else if(isa<ArraySubscriptExpr>(expression))
                expression=cast<ArraySubscriptExpr>(expression)->getBase();
        }
        assert(i<10);
    }
    return innerDecl;
}

const_arg* getInnerConstArg(Expr* expression )
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
const_arg* getInnerConstArg(ValueDecl* valDecl)
{
    return getHashTableValue(valDecl);   
} 
//Checks if the init part of a Variable Declaration contains a value
//Returns false if init is NULL or declaration is : type* a=new ...
//Returns true otherwise
bool valueInit(VarDecl* varDec)
{
    return (varDec->getInit()!=NULL&&!isa<CXXNewExpr>(varDec->getInit()));
}