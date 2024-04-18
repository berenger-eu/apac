#include "core.hpp"
using namespace clang;


Expr* getInnerPtr(Expr* expression)
{
    Expr* returnValueExpr=NULL;
    int i=0;    //To avoid loops and still constify the file 
    while(expression!=NULL&&returnValueExpr==NULL&&i<10)
    { 
        i++;
        if(isa<DeclRefExpr>(expression))
        {
            returnValueExpr=getInnerExpr(expression);
            expression=NULL;
        }
        else if(isa<MemberExpr>(expression))
        {
            returnValueExpr=cast<MemberExpr>(expression);
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
    
    return returnValueExpr;
}



//Retrieves, if it exists, the variable inside of an expression
Expr* getInnerExpr(Expr* expression)
{
    Expr* innerExpr=NULL;
    
    if(expression!=NULL)
    {
        int i=0;    //To avoid loops and still constify the file 
        while(expression!=NULL&&innerExpr==NULL&&i<10)
        {
            i++;
            expression=expression->IgnoreCasts();
            if(isa<DeclRefExpr>(expression))
                innerExpr=cast<DeclRefExpr>(expression);
            else if(isa<MemberExpr>(expression))
                innerExpr=cast<MemberExpr>(expression);
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
    return innerExpr;
}


//Checks if the init part of a Variable Declaration contains a value
//Returns false if init is NULL or declaration is : type* a=new ...
//Returns true otherwise
bool valueInit(VarDecl* varDec)
{
    return (varDec->getInit()!=NULL&&!isa<CXXNewExpr>(varDec->getInit()));
}




//true if the expression is the return of a function, false otherwise
bool isExprACall(Expr* expression)
{   
    bool result=false;
    if(expression!=NULL)
    {
        expression=expression->IgnoreCasts();
        if(isa<CallExpr>(expression))
            result=true;
    }
    return result;
}
