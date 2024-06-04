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
std::string getStmtAsString(const Stmt* statement,const LangOptions& langOpt)
{
    std::string stmtString;
    if(statement!=NULL)
    {
        std::stringstream SSprint;
        PrintingPolicy print_policy(langOpt);
        print_policy.SuppressUnwrittenScope=true;
        llvm::raw_string_ostream stringStreamStmt(stmtString);
        statement->printPretty(stringStreamStmt,NULL,print_policy);
    }
    return stmtString;
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
    bool returnValue=false;

    returnValue=qType->isReferenceType();
    if(!returnValue)
        returnValue=qType.getAsString().find("reference_wrapper")!=std::string::npos;

    return returnValue;
}

void getLeafs(Stmt* st,std::vector< Stmt*>& leafs)
{
    std::queue<Stmt*> vectNodes;
    vectNodes.push(st);
    int temp=0;
    while(!vectNodes.empty())
    {
        temp++;
        Stmt* s=vectNodes.front();
        int lastSize=vectNodes.size();
        if (isa<CallExpr>(s))
            leafs.push_back(s);
        else
        {
            for (auto it = s->child_begin(); it != s->child_end(); ++it) {
                vectNodes.push(*it);
            }
            if(lastSize==vectNodes.size())
                leafs.push_back(s);
        }
        vectNodes.pop();
    }
}
DeclRefExpr* getDeclRefExprInsideExpr(Expr* e)
{
    DeclRefExpr* returnValue=NULL;
    std::queue<Expr*> vectNodes;
    vectNodes.push(e);
    while(returnValue==NULL&&!vectNodes.empty())
    {
        Expr* s=vectNodes.front();
        if (isa<DeclRefExpr>(s))
            returnValue=cast<DeclRefExpr>(s);
        else
            for (auto it = s->child_begin(); it != s->child_end(); ++it)
                if(isa<Expr>(*it))
                    vectNodes.push(cast<Expr>(*it));
        vectNodes.pop();
    }
    return returnValue;
}
bool isFullConstType(const QualType& qType)
{
    const Type* typeTemp;
    bool workDone=false;
    bool returnValue=true;
    QualType qTypeTemp=qType;
    if(isReferenceQualType(qType))
        qTypeTemp=qType.getNonReferenceType();
    do
    {
        if(qTypeTemp.isConstQualified())
            qTypeTemp=qTypeTemp->getPointeeType();
        else
        {
            workDone=true;
            returnValue=false;
        }
    }
    while ((typeTemp=qTypeTemp.getTypePtrOrNull())&&!workDone);
    return returnValue;
}