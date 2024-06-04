#include "common.hpp"

std::string getCompleteVarDeclStr(const VarDecl& v){
  std::stringstream SSprint;
  SSprint<<v.getType().getAsString()<<" "<<getVarDeclDefStr(v);
  return SSprint.str();
}
std::string getVarDeclDefStr(const VarDecl& v)
{
    std::stringstream SSresult;
    //type varName
    if(v.getInit())
    {
        SSresult<<v.getNameAsString();
        //= initValue
        switch(v.getInitStyle())
    {
        case VarDecl::CallInit:
            SSresult<<"("<<getInitString(v)<<")";
            break;
        case VarDecl::CInit:
            SSresult<<" = "<<getInitString(v);
            break;
        default:
            llvm::errs()<<"Unknown Initialization Style\n";
            break;
    }
        SSresult<<";\n";
    }
    return SSresult.str();
}
std::string getVarDeclDeclStr(const VarDecl& v)
{
    std::stringstream SSresult;
    //type varName
    SSresult<<v.getType().getAsString()<<" "<<v.getNameAsString()<<";\n";
    return SSresult.str();
}
std::string getExprAsString(const Expr* expression,const LangOptions& langOpt)
{
    std::string exprString;
    if(expression!=NULL)
    {

        std::stringstream SSprint;
        PrintingPolicy print_policy(langOpt);
        print_policy.SuppressUnwrittenScope=true;
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
