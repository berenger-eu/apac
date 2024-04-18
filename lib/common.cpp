#include "common.hpp"

std::string getCompleteVarDeclStr(VarDecl& v)
{
    std::stringstream SSresult;
    //type varName
    SSresult<<v.getType().getAsString()<<" "<<v.getNameAsString();
    if(v.getInit())
    {
        //= initValue
        SSresult<<" = "<<getInitString(v);
    }
    SSresult<<";\n";
    return SSresult.str();
}
std::string getExprAsString(const Expr* expression,const LangOptions& langOpt)
{
    std::string exprString;
    if(expression!=NULL)
    {
        std::stringstream SSprint;
        PrintingPolicy print_policy(langOpt);
        llvm::raw_string_ostream stringStreamExpr(exprString);
        expression->printPretty(stringStreamExpr,NULL,print_policy);
    }
    return exprString;
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
