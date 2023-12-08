#include "../include/core.hpp"
using namespace clang;
//HashKey is FunctionName++VarName or VarName
std::string getHashKey(NamedDecl* nd)
{
    std::stringstream SSConcatStrings;
    if(nd->getDeclContext()->isFunctionOrMethod())
        SSConcatStrings<<cast<FunctionDecl>(nd->getDeclContext())->getNameAsString();
    std::string varName=nd->getQualifiedNameAsString();
    SSConcatStrings<<varName;
    return SSConcatStrings.str();
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



ValueDecl* getInnerPtr(Expr* expression)
{
    ValueDecl* returnValueDecl=NULL;
    while(expression!=NULL&&!isa<DeclRefExpr>(expression))
    { 
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
        else
            expression=expression->IgnoreCasts();            
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
        }
        innerDecl=cast<DeclRefExpr>(expression)->getDecl();
    }
    return innerDecl;
}