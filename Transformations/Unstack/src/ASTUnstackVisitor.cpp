#include "ASTUnstackVisitor.hpp"
using namespace clang;
//Mostly here to reset the counter for variables, we assume that number will not overflow
bool ASTUnstackVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(fDecl->getBeginLoc()))
        return true;

    tempVarsCounter=0;
    return true;
}
bool ASTUnstackVisitor::VisitCompoundStmt(CompoundStmt* coSt)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(coSt->getBeginLoc()))
        return true;
    for (CompoundStmt::body_iterator b = coSt->body_begin(), e = coSt->body_end()
    ; b != e; ++b)
        handleSubStmt((*b));
    return true;

}
void ASTUnstackVisitor::handleSubStmt(Stmt* st)
{
    if(st==NULL)
        ;
    else if(isa<BinaryOperator>(st)){
        subVisitBinaryOperator(cast<BinaryOperator>(st));
    }
    else if(isa<UnaryOperator>(st)){
        subVisitUnaryOperator(cast<UnaryOperator>(st));
    }   
    else if(isa<DeclStmt>(st)){
        subVisitDeclStmt(cast<DeclStmt>(st));
    }
    
    else if(isa<CallExpr>(st)){
        subVisitCallExpr(cast<CallExpr>(st));
    }
    
}
void ASTUnstackVisitor::subVisitCallExpr(CallExpr* calExpr)
{
    std::vector<Expr*> exprList {calExpr};
    transfoInstruction(exprList,calExpr);
}
void ASTUnstackVisitor::subVisitBinaryOperator(BinaryOperator* bop)
{
    std::vector<Expr*> exprList {bop->getLHS(),bop->getRHS()};
    transfoInstruction(exprList,bop);
}
void ASTUnstackVisitor::subVisitUnaryOperator(UnaryOperator* uop)
{
    std::vector<Expr*> exprList {uop->getSubExpr()};
    transfoInstruction(exprList,uop);
}
void ASTUnstackVisitor::subVisitDeclStmt(DeclStmt* declSt)
{    
    std::stringstream SSprint;
    if((declSt->isSingleDecl()))
    {
        Decl* d=declSt->getSingleDecl();
        if(isa<VarDecl>(d))
        {
            VarDecl* varDec=cast<VarDecl>(d);
            std::vector<Expr*> exprList {varDec->getInit()};
            transfoInstruction(exprList,declSt);
         }
    }
    //Multiple decl
    //Ignored for now
    else
    {
        DeclGroup& dgr=declSt->getDeclGroup().getDeclGroup();
        int dgrSize=dgr.size();
        std::vector<Expr*> exprList;
        for(int i=0;i<dgrSize;i++)
            if(isa<VarDecl>(dgr[i]))
            {
                VarDecl* vd =cast<VarDecl>(dgr[i]);
                exprList.push_back(vd->getInit());
            }
        transfoInstruction(exprList,declSt);
    }
}
//Transform a CallExpr into a varTemp and adds the unstacked call before instructionBegin

void ASTUnstackVisitor::unstackTransformCallExpr(CallExpr* calExp,SourceLocation instructionBegin)
{
    std::vector<CallExpr*> vectCallExpr;
    recursiveStringCreateTransformCallExpr(calExp,vectCallExpr);
    std::stringstream SScall;
    std::queue<int>tempVarQueue;
    for(auto b=vectCallExpr.begin(),e=vectCallExpr.end();b!=e;b++)
    {
        tempVarQueue.push(tempVarsCounter);
        SScall<<createTempVarString(*b,tempVarsCounter,tempVarQueue);
        tempVarsCounter++;
    }
    std::stringstream SSvar;
    SSvar<<"__tempVar_"<<tempVarsCounter-1;
    TheRewriter.ReplaceText(SourceRange(calExp->getBeginLoc(),calExp->getEndLoc()),SSvar.str());  
    TheRewriter.InsertTextBefore(instructionBegin,SScall.str());
}
//Returns all top CallExpr within an Expr
void ASTUnstackVisitor::findTopCallsInExpr(Expr* ex,std::vector<CallExpr*>& callExprVect)
{
    if(ex==NULL)
        return;
    ex=ex->IgnoreImpCasts();
    if(isa<CallExpr>(ex))
    {
        callExprVect.push_back(cast<CallExpr>(ex));
    }
    else if(isa<BinaryOperator>(ex))
    {
        BinaryOperator* bop=cast<BinaryOperator>(ex);
        findTopCallsInExpr(bop->getLHS(),callExprVect);
        findTopCallsInExpr(bop->getRHS(),callExprVect);
    }
    else if(isa<UnaryOperator>(ex))
    {
        UnaryOperator* uop=cast<UnaryOperator>(ex);
        findTopCallsInExpr(uop->getSubExpr(),callExprVect);
    }
    //TODO:Add cases like BinaryOp,UnaryOp, ...
}

void ASTUnstackVisitor::recursiveStringCreateTransformCallExpr(CallExpr* calExp,std::vector<CallExpr*>& vectCallExpr)
{
    for(auto beg=calExp->arg_begin(),end=calExp->arg_end();beg!=end;beg++)
    {
        Expr* arg=(*beg);
        std::vector<CallExpr*> callsInArg;
        findTopCallsInExpr(arg,callsInArg);
        for(auto b=callsInArg.begin(),e=callsInArg.end();b!=e;b++)
            recursiveStringCreateTransformCallExpr(*b,vectCallExpr);
    }
    //SSprint<<createTempVarString(calExp,tempVarsCounter,argNumbers);
    //tempVarsCounter++;
    vectCallExpr.push_back(calExp);
}

std::string ASTUnstackVisitor::createCallArgString(Expr* argExpr,std::queue<int>& tempVarQueue)
{
    std::stringstream res;
    if(argExpr==NULL)
        ;
    argExpr=argExpr->IgnoreImpCasts();
    if(isa<CallExpr>(argExpr))
    {
        res<<" __tempVar_"<<tempVarQueue.front()<<" ";
        tempVarQueue.pop(); 
    }
    else if(isa<BinaryOperator>(argExpr))
    {
        BinaryOperator* bop=cast<BinaryOperator>(argExpr);
        res<<createCallArgString(bop->getLHS(),tempVarQueue)
        <<bop->getOpcodeStr().str() 
        <<createCallArgString(bop->getRHS(),tempVarQueue);
    }
    else if(isa<UnaryOperator>(argExpr))
    {
        UnaryOperator* uop=cast<UnaryOperator>(argExpr);
        std::string opcodeStr;
        if(uop->isDecrementOp())
            opcodeStr="--";
        else 
            opcodeStr="++";
        if(uop->isPostfix())
            res<<createCallArgString(uop->getSubExpr(),tempVarQueue)
            <<opcodeStr;
        else
            res<<opcodeStr
            <<createCallArgString(uop->getSubExpr(),tempVarQueue);
    }
    else
    {
        res<<getExprAsString(argExpr);
    }
    return res.str();
}

//Create string : type __tempVar_x; __tempVar_x = unstackedCall; 
std::string ASTUnstackVisitor::createTempVarString(CallExpr* calExp,int currentCounterNumber,std::queue<int>& tempVarQueue)
{
    std::stringstream SSresult;
    //Get the return type of the function
    ASTContext& aCons=calExp->getDirectCallee()->getASTContext();
    std::string functionType=calExp->getCallReturnType(aCons).getAsString();
    //Type __tempVar_x;
    SSresult<<functionType<<" __tempVar_"<<currentCounterNumber
    //= functionName ( 
    <<" = " <<calExp->getDirectCallee()->getNameAsString()<<"(";
    int currentArg=0;
    bool firstArg=true;
    //Prints all of its arguments 
    for(auto b=calExp->arg_begin(),e=calExp->arg_end();b!=e;b++)
    {
        if(firstArg)
        {
            firstArg=false;
            SSresult<<createCallArgString(*b,tempVarQueue);
        }
        else
            SSresult<<","<<createCallArgString(*b,tempVarQueue);
        
    }
    SSresult<<");\n";
    return SSresult.str();
}

void ASTUnstackVisitor::transfoInstruction(std::vector<Expr*>& exprVect,Stmt* instruction)
{
    for(auto beg=exprVect.begin(),end=exprVect.end();beg!=end;beg++)
        tranfoExpr(*beg,instruction);
}
void ASTUnstackVisitor::tranfoExpr(Expr* ex,Stmt* instruction)
{
    std::vector<CallExpr*> callList;
    findTopCallsInExpr(ex,callList);
    for(auto beg=callList.begin();beg!=callList.end();beg++)
        unstackTransformCallExpr(*beg,instruction->getBeginLoc());

}
std::string ASTUnstackVisitor::getExprAsString(Expr* expression)
{
    std::stringstream SSprint;
    PrintingPolicy print_policy(TheRewriter.getLangOpts());
    std::string exprString;
    llvm::raw_string_ostream stringStreamExpr(exprString);
    expression->printPretty(stringStreamExpr,NULL,print_policy);
    return exprString;
}