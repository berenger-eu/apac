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


//Retrieves, if it exists, the variable inside of an expression
ValueDecl* getInnerDecl(Expr* expression)
{
    ValueDecl* innerDecl=NULL;
    if(expression!=NULL)
    {
        while(!isa<DeclRefExpr>(expression)&&expression!=NULL)
        {
            expression=expression->IgnoreCasts();
            if(isa<UnaryOperator>(expression))
                expression=cast<UnaryOperator>(expression)->getSubExpr();
        }
        innerDecl=cast<DeclRefExpr>(expression)->getDecl();
    }
    return innerDecl;
}