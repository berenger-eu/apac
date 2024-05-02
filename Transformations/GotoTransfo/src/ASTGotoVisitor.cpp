#include "ASTGotoVisitor.hpp"
using namespace clang;
bool ASTGotoVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
    if(isInHeaders(TheRewriter.getSourceMgr(),fDecl->getBeginLoc())
    ||!fDecl->isThisDeclarationADefinition())
        return true;
    //Declares the result in the transformed source file, only if it's not void
    if(!fDecl->getReturnType().getTypePtr()->isVoidType())
    {
        std::stringstream SSprint;
        //Declares the result
        SSprint<<"\nstd::unique_ptr<"<<fDecl->getReturnType().getAsString(TheRewriter.getLangOpts())<<"> __result;\n";
        TheRewriter.InsertTextAfterToken(fDecl->getBody()->getBeginLoc(),SSprint.str());
    }

    //Adds the exit section
    std::stringstream SSexit;
    //TODO:Handle other cases with no returns 
    if(!fDecl->getReturnType().getTypePtr()->isVoidType())
        SSexit<<"__exit"<<functionsCounter<<": return *__result;\n";
    else
        SSexit<<"__exit"<<functionsCounter<<": return ;\n";
    TheRewriter.InsertTextAfter(fDecl->getBody()->getEndLoc(),SSexit.str());
    Stmt* fDeclBody=fDecl->getBody();
    assert(fDeclBody&&"Function body is null\n");
    if( fDeclBody&&isa<CompoundStmt>(fDecl->getBody()))
        subVisitCompoundStmt(cast<CompoundStmt>(fDeclBody));
    functionsCounter++;
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
    //Replaces return with __result=std::make_unique<type>(); if there is a return value   
    if(retStmt.getRetValue())
        SSprint<<"__result = std::make_unique<"<<retStmt.getRetValue()->getType().getAsString(TheRewriter.getLangOpts())<<">("
        <<getExprAsString(retStmt.getRetValue(), TheRewriter.getLangOpts())<<");\n";
    // goto __exitX;\n
    SSprint<<"goto __exit"<<exitCounter;
    return SSprint.str();
}   