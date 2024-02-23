#include "../include/core.hpp"
using namespace clang;

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
        else if(isa<CXXNewExpr>(expression))
            expression=NULL;     
        else
            expression=expression->IgnoreParenCasts();

    }
    if(i>=10){
        expression->dump();
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
            else if(isa<CXXNewExpr>(expression))
                expression=NULL;    
        }
        if(i>=10){
            expression->dump();
        }
        
        assert(i<10);
    }
    return innerDecl;
}


//Checks if the init part of a Variable Declaration contains a value
//Returns false if init is NULL or declaration is : type* a=new ...
//Returns true otherwise
bool valueInit(VarDecl* varDec)
{
    return (varDec->getInit()!=NULL&&!isa<CXXNewExpr>(varDec->getInit()));
}