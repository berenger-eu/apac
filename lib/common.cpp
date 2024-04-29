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
        SSresult<<" = "<<getInitString(v);
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

void getLeafs(Stmt* st,std::vector< Stmt*>& leafs)
{
    std::queue<Stmt*> vectNodes;
    vectNodes.push(st);
    int temp=0;
    while(!vectNodes.empty()&&temp<10)
    {
        temp++;
        Stmt* s=vectNodes.front();
        int lastSize=vectNodes.size();
        for (auto it = s->child_begin(); it != s->child_end(); ++it) {
            vectNodes.push(*it);
        }
        if(lastSize==vectNodes.size())
            leafs.push_back(s);
        vectNodes.pop();
    }
}