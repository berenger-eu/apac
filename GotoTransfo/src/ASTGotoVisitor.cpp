#include "ASTGotoVisitor.hpp"
using namespace clang;
bool ASTGotoVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(fDecl->getBeginLoc()))
        return true;

    functionsCounter++;

    std::stringstream SSprint;
    //Declares the result
    SSprint<<"\n"<<fDecl->getReturnType().getAsString()<<" __result;\n";
    TheRewriter.InsertTextAfterToken(fDecl->getBody()->getBeginLoc(),SSprint.str());

    //Adds the exit section
    std::stringstream SSexit;
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
        handleSubStmt((*b));
}
void ASTGotoVisitor::handleSubStmt(Stmt* st)
{
    if (st==NULL)
        ;
    else if(isa<ReturnStmt>(st))
        subVisitReturnStmt(cast<ReturnStmt>(st));
    else if (isa<CompoundStmt>(st))
        subVisitCompoundStmt(cast<CompoundStmt>(st));
    //Will create a scope possibly outside of a compound stmt
    else if (isa<IfStmt>(st))
        subVisitIfStmt(cast<IfStmt>(st));
    else if (isa<ForStmt>(st))
        subVisitForStmt(cast<ForStmt>(st));
    else if (isa<WhileStmt>(st))
        subVisitWhileStmt(cast<WhileStmt>(st));
}

void ASTGotoVisitor::subVisitReturnStmt(ReturnStmt* retStmt)
{
    //Retrieve the value of the return as a string
    TheRewriter.ReplaceText(SourceRange(retStmt->getBeginLoc(),retStmt->getEndLoc()),createGotoString((*retStmt),TheRewriter,functionsCounter));
}
void ASTGotoVisitor::subVisitForStmt(ForStmt* forSt)
{
    Stmt* bodySt=forSt->getBody();
    if(bodySt&&isa<ReturnStmt>(bodySt))
    {
        
    }
    else 
        handleSubStmt(forSt->getBody());
}
void ASTGotoVisitor::subVisitWhileStmt(WhileStmt* whileSt)
{
    Stmt* bodySt=whileSt->getBody();
    if(bodySt&&isa<ReturnStmt>(bodySt))
    {

    }
    else 
        handleSubStmt(whileSt->getBody());
}

void ASTGotoVisitor::subVisitIfStmt(IfStmt* ifSt)
{
    Stmt* subSt=ifSt->getThen();
    
    if(subSt&&isa<ReturnStmt>(subSt))
    {
        std::stringstream SSprint;
        ReturnStmt* retStmt=cast<ReturnStmt>(subSt);
        SSprint<<"{\n"<<createGotoString((*retStmt),TheRewriter,functionsCounter);
        TheRewriter.ReplaceText(SourceRange(subSt->getBeginLoc(),subSt->getEndLoc()),SSprint.str());
        TheRewriter.InsertTextAfterToken(subSt->getEndLoc().getLocWithOffset(1),"\n}");
    }
    else
        handleSubStmt(subSt);
    subSt=ifSt->getElse();
    if(subSt&&isa<ReturnStmt>(subSt))
    {
        std::stringstream SSprint;
        ReturnStmt* retStmt=cast<ReturnStmt>(subSt);
        SSprint<<"{\n"<<createGotoString((*retStmt),TheRewriter,functionsCounter);
        TheRewriter.ReplaceText(SourceRange(subSt->getBeginLoc(),subSt->getEndLoc()),SSprint.str());
        TheRewriter.InsertTextAfterToken(subSt->getEndLoc().getLocWithOffset(1),"\n}");
    }
    else
        handleSubStmt(subSt);
}

std::string createGotoString(ReturnStmt& retStmt,Rewriter& TheRewriter,unsigned int& exitCounter)
{
    std::stringstream SSprint;
    PrintingPolicy print_policy(TheRewriter.getLangOpts());
    std::string initString;
    llvm::raw_string_ostream stringStreamInit(initString);
    retStmt.getRetValue()->printPretty(stringStreamInit,NULL,print_policy);

    //Replaces return with __result=<returnValue>;goto __exitX;\n
    SSprint<<"__result = "<<initString<<";\ngoto __exit"<<exitCounter;
    //TheRewriter.ReplaceText(SourceRange(retStmt->getBeginLoc(),retStmt->getEndLoc()),SSprint.str());
    //TheRewriter.InsertTextAfterToken(retStmt->getEndLoc(),"\n}");
    return SSprint.str();
}   