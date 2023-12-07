#include "../include/core.hpp"
using namespace clang;
std::string getHashKey(NamedDecl* nd)
{
    std::stringstream SSConcatStrings;
    if(nd->getDeclContext()->isFunctionOrMethod())
        SSConcatStrings<<cast<FunctionDecl>(nd->getDeclContext())->getNameAsString();
    std::string varName=nd->getQualifiedNameAsString();
    SSConcatStrings<<varName;
    return SSConcatStrings.str();
}
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