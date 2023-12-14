#include "../include/core.hpp"
using namespace clang;
const_arg* getHashTableValue (NamedDecl* nd)
{
    const_arg* tableValue=NULL;
    if(nd!=NULL)
    {
        std::string key=getHashKey(nd);
        tableValue=&(const_arg_table[key]);
    }
    return tableValue;
}
//HashKey is Namespaces::FunctionName::VarName or VarName
std::string getHashKey(NamedDecl* nd)
{
    std::stringstream SSresult;
    std::stringstream SSnamespaces;
    std::string funcStr="";
    std::string varName=nd->getQualifiedNameAsString();;
    std::string namespaceNameStr="";
    DeclContext* varDeclContext=nd->getDeclContext();
    if(varDeclContext->isFunctionOrMethod())
    {
        funcStr=cast<FunctionDecl>(nd->getDeclContext())->getNameAsString()+"::";
        varDeclContext=varDeclContext->getParent();
    }
    while (varDeclContext!=NULL&&varDeclContext->isNamespace())
    {
        std::string namespaceNameTemp=cast<NamespaceDecl>(varDeclContext)->getNameAsString();
        SSnamespaces<<"::"<<namespaceNameTemp;
        varDeclContext=varDeclContext->getParent();
    }
    namespaceNameStr=SSnamespaces.str();

    std::reverse(namespaceNameStr.begin(),namespaceNameStr.end());
    SSresult<<namespaceNameStr<<funcStr <<varName;
    return SSresult.str();
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
        //Calls to functions break the function, and can't be linked to a variable
        else if(isa<CallExpr>(expression))
            expression=NULL;
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
            //We can't get the variable from a call to a function
            else if(isa<CallExpr>(expression))
                expression=NULL;
        }
        innerDecl=cast<DeclRefExpr>(expression)->getDecl();
    }
    return innerDecl;
}