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
    if(!statement)
        return std::string();
    if(isa<ForStmt>(statement))
    {
        const ForStmt* f=cast<ForStmt>(statement);
        std::stringstream ss;
        ss<<"for("<<getStmtAsString(f->getInit(),langOpt)<<";"
        <<getExprAsString(f->getCond(),langOpt)<<";"
        <<getExprAsString(f->getInc(),langOpt)<<")";
        return ss.str();
    }
    else if(isa<IfStmt>(statement))
    {
        const IfStmt* i=cast<IfStmt>(statement);
        std::stringstream ss;
        ss<<"if("<<getExprAsString(i->getCond(),langOpt)<<")";
        return ss.str();
    }
    else
    {
        std::string stmtString;
        std::stringstream SSprint;
        PrintingPolicy print_policy(langOpt);
        print_policy.SuppressUnwrittenScope=true;
        llvm::raw_string_ostream stringStreamStmt(stmtString);
        statement->printPretty(stringStreamStmt,NULL,print_policy);
        return stmtString;
    }
}
std::string getStmtAsString(const ForStmt* f,const LangOptions& langOpt)
{
    
}
std::string getStmtAsString(const IfStmt* i,const LangOptions& langOpt)
{
    
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
        auto lastSize=vectNodes.size();
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
std::vector<const DeclRefExpr*> getAllDeclRefExprInsideExpr(const Expr* e)
{
    if(!e)
        return std::vector<const DeclRefExpr*>();
    std::vector<const DeclRefExpr*> vectDeclRefExpr;
    std::queue<const Expr*> vectNodes;
    vectNodes.push(e);
    while(!vectNodes.empty())
    {
        const Expr* s=vectNodes.front();
        if (isa<DeclRefExpr>(s)&&isa<VarDecl>(cast<DeclRefExpr>(s)->getDecl()))
            vectDeclRefExpr.push_back(cast<DeclRefExpr>(s));
        else
            for (auto it = s->child_begin(); it != s->child_end(); ++it)
                if(isa<Expr>(*it))
                    vectNodes.push(cast<Expr>(*it));
        vectNodes.pop();
    }
    return vectDeclRefExpr;
}
const DeclRefExpr* getSingleDeclRefExprInsideExpr(const Expr* e)
{
    const DeclRefExpr* returnValue=NULL;
    std::vector<const DeclRefExpr*> vectDeclRefExpr=getAllDeclRefExprInsideExpr(e);
    if(vectDeclRefExpr.size()==1)
        returnValue=vectDeclRefExpr.front();
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

int getPtrDepthAccess(QualType qt1, QualType qt2,const ASTContext& aContext){
    int returnValue=0;
    if(qt1!=qt2)
    {
        if(getPointerToQType(qt1,aContext)==qt2)
            returnValue=-1;
        else
            while(qt1!=qt2&&qt1.getTypePtrOrNull()&&qt2.getTypePtrOrNull()&&qt1->getPointeeType()!=qt2->getPointeeType())
            {
                if(isPointerQualType(qt1))
                {
                    returnValue++;
                    qt1=qt1->getPointeeType();
                }
            }
    }
    return returnValue;
}

int getPtrDepthAccess(const clang::VarDecl& v,const clang::Expr& e)
{
    const QualType& qt1=v.getType();
    const QualType& qt2=e.getType();
    return getPtrDepthAccess(qt1,qt2,v.getASTContext());
}
