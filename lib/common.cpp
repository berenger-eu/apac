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
    //Check for simple pointers type
    if((typeTemp=qType.getTypePtrOrNull()))
    {
        returnValue=typeTemp->isPointerType();
    }
    //TODO:Check for more complex pointer types (shared_ptr,unique_ptr,...)
    return returnValue;
}
bool isReferenceQualType(QualType qType)
{
    bool returnValue=false;

    returnValue=qType->isReferenceType();
    //We check for special cases where the type is not a simple reference type
    //Here, we check if it's a reference_wrapper
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
    //We remove the reference if there is one, since it's supposed to be const
    if(isReferenceQualType(qType))
        qTypeTemp=qType.getNonReferenceType();
    //We check all types until we reach the last one, we stop if we find a non-const type
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
    //If both types are the same, then we return 0 since they have the same depth 
    if(qt1!=qt2)
    {
        //If a pointer to type qt1 is equal to qt2, then qt2 is a pointer to type qt1
        if(getPointerToQType(qt1,aContext)==qt2)
            returnValue=-1;
        //Else, we compare type pointed by qt1 until we find the same type as qt2
        //The number of iteration is the depth of the pointer access
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
