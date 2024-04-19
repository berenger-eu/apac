#include "ASTGotoVisitor.hpp"
using namespace clang;
bool ASTGotoVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
    if(isInHeaders(TheRewriter.getSourceMgr(),fDecl->getBeginLoc()))
        return true;

    functionsCounter++;

    std::stringstream SSprint;
    //Declares the result
    SSprint<<"\n"<<fDecl->getReturnType().getAsString()<<" __result;\n";
    TheRewriter.InsertTextAfterToken(fDecl->getBody()->getBeginLoc(),SSprint.str());

    //Adds the exit section
    std::stringstream SSexit;
    //TODO:Handle void result or cases with no returns 
    SSexit<<"__exit"<<functionsCounter<<": return __result;\n";
    TheRewriter.InsertTextAfter(fDecl->getBody()->getEndLoc(),SSexit.str());
    Stmt* fDeclBody=fDecl->getBody();
    if( fDeclBody&&isa<CompoundStmt>(fDecl->getBody()))
        subVisitCompoundStmt(cast<CompoundStmt>(fDeclBody));
    return true;
}

void ASTGotoVisitor::subVisitCompoundStmt(CompoundStmt* coSt)
{
    for (CompoundStmt::body_iterator b = coSt->body_begin(), e = coSt->body_end(); b != e; ++b)
        handleSubStmt(*b);    
}

void ASTGotoVisitor::handleSubStmt(Stmt* st)
{
    if (st==NULL)
    ;
    else if(isa<ReturnStmt>(st))
        subVisitReturnStmt(cast<ReturnStmt>(st));
    else if (isa<CompoundStmt>(st))
        subVisitCompoundStmt(cast<CompoundStmt>(st));
    else if (isa<IfStmt>(st))
        subVisitIfStmt(cast<IfStmt>(st));
    else if (isa<ForStmt>(st))
        subVisitForStmt(cast<ForStmt>(st));
    else if (isa<WhileStmt>(st))
        subVisitWhileStmt(cast<WhileStmt>(st));
}

void ASTGotoVisitor::subVisitReturnStmt(ReturnStmt* retStmt)
{
    //Insert a Goto and affect the value of the return to result
    TheRewriter.ReplaceText(SourceRange(retStmt->getBeginLoc(),retStmt->getEndLoc())
    ,createGotoString((*retStmt),TheRewriter,functionsCounter));
}

std::string createGotoString(const ReturnStmt& retStmt,const Rewriter& TheRewriter,const unsigned int& exitCounter)
{
    std::stringstream SSprint;
    //Replaces return with __result=<returnValue>;goto __exitX;\n
    SSprint<<"__result = "<<getExprAsString(retStmt.getRetValue(), TheRewriter.getLangOpts())
    <<";\ngoto __exit"<<exitCounter;
    return SSprint.str();
}   