#include "ASTUnstackVisitor.hpp"
using namespace clang;
//Mostly here to reset the counter for variables, we assume that number will not overflow
bool ASTUnstackVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
    if(isInHeaders(TheRewriter.getSourceMgr(),fDecl->getBeginLoc()))
        return true;

    tempVarsCounter=0;
    return true;
}
bool ASTUnstackVisitor::VisitCompoundStmt(CompoundStmt* coSt)
{
    if(isInHeaders(TheRewriter.getSourceMgr(),coSt->getBeginLoc()))
        return true;
    for (CompoundStmt::body_iterator b = coSt->body_begin(), e = coSt->body_end()
    ; b != e; ++b)
        handleSubStmt((*b));
    return true;

}
void ASTUnstackVisitor::handleSubStmt(Stmt* st)
{
    std::vector<Expr*> exprList;
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
    else{
        llvm::errs()<<"Statement is not handled\n";
    }
    
}

void ASTUnstackVisitor::subVisitDeclStmt(DeclStmt* declSt)
{    
    std::stringstream SSprint;
    //Vector containing all expressions that might need to be transformed by the transformation
    std::vector<Expr*> exprList;
    if((declSt->isSingleDecl()))
    {
        Decl* d=declSt->getSingleDecl();
        if(isa<VarDecl>(d))
        {
            VarDecl* varDec=cast<VarDecl>(d);
            exprList.push_back (varDec->getInit());
            transfoInstruction(exprList,declSt->getBeginLoc());
         }
    }
    //Multiple decl
    else
    {
        const DeclGroupRef& dgr=declSt->getDeclGroup();
        //We add the Init part of each declaration to the list of expression
        for(DeclGroupRef::const_iterator b=dgr.begin(),e=dgr.end();b!=e;b++)
            if(isa<VarDecl>(*b))
                exprList.push_back((cast<VarDecl>(*b))->getInit());
        transfoInstruction(exprList,declSt->getBeginLoc());
    }
}

//Transform a CallExpr into a varTemp and adds the unstacked call before instructionBegin
void ASTUnstackVisitor::unstackTransformCallExpr(CallExpr* calExp,const SourceLocation& instructionBegin)
{
    std::vector<CallExpr*> vectCallExpr;
    findAllCallExpr(calExp,vectCallExpr);
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

void ASTUnstackVisitor::findAllCallExpr(CallExpr* calExp,std::vector<CallExpr*>& vectCallExpr)
{
    //We look for the CallExpr in each Argument
    for(auto beg=calExp->arg_begin(),end=calExp->arg_end();beg!=end;beg++)
    {
        Expr* arg=(*beg);
        std::vector<CallExpr*> callsInArg;
        //Find all top CallExpr 
        findTopCallsInExpr(arg,callsInArg);
        //Then look through them 
        for(auto b=callsInArg.begin(),e=callsInArg.end();b!=e;b++)
            findAllCallExpr(*b,vectCallExpr);
    }
    vectCallExpr.push_back(calExp);
}

std::string ASTUnstackVisitor::createCallArgString(Expr* argExpr,std::queue<int>& tempVarQueue)
{
    std::stringstream res;
    if(argExpr==NULL)
        ;
    argExpr=argExpr->IgnoreImpCasts();
    //If it's a call, we can replace it by the corresponding variable
    if(isa<CallExpr>(argExpr))
    {
        res<<" __tempVar_"<<tempVarQueue.front()<<" ";
        tempVarQueue.pop(); 
    }
    //If it's a BinaryOperator, then we have to look at both expressions
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
    //Either the Expr does not contain a call, or we haven't found it
    //So we don't change anything either way 
    else
    {
        res<<getExprAsString(argExpr,TheRewriter.getLangOpts());
    }
    return res.str();
}

//Create string : type __tempVar_x; __tempVar_x = unstackedCall; 
std::string ASTUnstackVisitor::createTempVarString(CallExpr* calExp,int currentCounterNumber,std::queue<int>& tempVarQueue)
{
    std::stringstream SSresult;
    //Get the return type of the function
    ASTContext& aCons=calExp->getDirectCallee()->getASTContext();
    std::string functionType=calExp->getCallReturnType(aCons).getAsString(TheRewriter.getLangOpts());
    //Type __tempVar_x;
    SSresult<<functionType<<" __tempVar_"<<currentCounterNumber
    //= functionName ( 
    <<" = " <<calExp->getDirectCallee()->getNameAsString()<<"(";
    int currentArg=0;
    bool firstArg=true;
    //Prints all of its arguments 
    for(auto b=calExp->arg_begin(),e=calExp->arg_end();b!=e;b++){
        if(!firstArg)
            SSresult<<","<<createCallArgString(*b,tempVarQueue);
        else{
            SSresult<<createCallArgString(*b,tempVarQueue);
            firstArg=false;
        }
        
        
    }
    SSresult<<");\n";
    return SSresult.str();
}

void ASTUnstackVisitor::transfoInstruction(std::vector<Expr*>& exprVect,const SourceLocation& instructionBeginLoc)
{
    for(auto beg=exprVect.begin(),end=exprVect.end();beg!=end;beg++)
        tranfoExpr(*beg,instructionBeginLoc);
}
void ASTUnstackVisitor::tranfoExpr(Expr* ex,const SourceLocation& instructionBeginLoc)
{
    std::vector<CallExpr*> callList;
    findTopCallsInExpr(ex,callList);
    for(auto beg=callList.begin();beg!=callList.end();beg++)
        unstackTransformCallExpr(*beg,instructionBeginLoc);

}
