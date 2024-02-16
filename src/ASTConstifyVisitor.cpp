#include "../include/ASTConstifyVisitor.hpp"  
using namespace clang;
bool ASTConstifyVisitor::VisitStmt(Stmt *s)
    {
        return true;
    }

bool ASTConstifyVisitor::VisitBinaryOperator(BinaryOperator *bop)
    {
        if(TheRewriter.getSourceMgr().isInSystemHeader(bop->getBeginLoc()))
        return true;

        if (bop->isAssignmentOp())
        {
            ValueDecl* leftSideDecl=getInnerDecl(bop->getLHS());
            // rightSide=cast<DeclRefExpr>(bop->getRHS());
            const_arg *curArg = getHashTableValue(leftSideDecl);
            //Variable on the left side is modified, so we unconst it
            unconstifyByPropagation(curArg);
        }

        return true;
    }
bool ASTConstifyVisitor::VisitUnaryOperator(UnaryOperator* uop)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(uop->getBeginLoc()))
        return true;
    //Unary operators which modify the variable    
    if(uop->isIncrementDecrementOp())
    {   
        ValueDecl* innerDecl=getInnerDecl(uop->getSubExpr());
        unconstifyByPropagation(getHashTableValue(innerDecl));
    }

    return true;
}

bool ASTConstifyVisitor::VisitReturnStmt(ReturnStmt* retStmt)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(retStmt->getBeginLoc()))
        return true;
    Expr* retValue=retStmt->getRetValue();
    //Returning a pointer might lead to modifications, so we have to unconst
    if(retValue!=NULL&&isPointerQualType(retValue->getType()))
    {
        ValueDecl* retValDecl=getInnerPtr(retValue);
        unconstifyByPropagation(getHashTableValue(retValDecl));
    }
    //Returning a reference might lead to modifications, so we have to unconst
    else if(retValue!=NULL&&isa<DeclRefExpr>(retValue))
    {
        //We cast it to its decl because the QualType of retValue for a reference will be int
        ValueDecl* retDecl=cast<DeclRefExpr>(retValue)->getDecl();
        if(isReferenceQualType(retDecl->getType()))
        {
            ValueDecl* retValDecl=getInnerDecl(retValue);
            unconstifyByPropagation(getHashTableValue(retValDecl));
        }
    }
    return true;
}

/*
bool ASTConstifyVisitor::VisitCXXMemberCallExpr(CXXMemberCallExpr* memExpr)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(memExpr->getBeginLoc()))
        return true;
    llvm::outs()<<"ICI3";
    if(isa<CXXMethodDecl>(memExpr->getMethodDecl()))
    {
        CXXMethodDecl* methDecl=cast<CXXMethodDecl>(memExpr->getMethodDecl());
        if(!methDecl->isConst())
        {
            memExpr->getCallee()->dump();
            //unconstifyByPropagation(getHashTableValue(getInnerPtr(memExpr->getBase())));
        }
    }
    return true;
}
*/
bool ASTConstifyVisitor::VisitCallExpr(CallExpr* ce)
{
    
    if(TheRewriter.getSourceMgr().isInSystemHeader(ce->getBeginLoc()))
        return true;
        if(isa<CXXMemberCallExpr>(ce))
        {
            CXXMemberCallExpr* memCallExpr=cast<CXXMemberCallExpr>(ce);
            CXXMethodDecl* methDecl=memCallExpr->getMethodDecl();
            if(!methDecl->isConst())
            {
                ValueDecl* innerDecl=getInnerDecl(cast<MemberExpr>(memCallExpr->getCallee())->getBase());
                unconstifyByPropagation(getHashTableValue(innerDecl));
                //unconstifyByPropagation(getHashTableValue(getInnerPtr(memExpr->getBase())));
            }
            return true;
        }
    FunctionDecl* fdec;
    assert(ce->getDirectCallee()!=NULL);
    if((fdec=ce->getDirectCallee())!=NULL&&TheRewriter.getSourceMgr().isInSystemHeader(fdec->getBeginLoc()))
    {   
        for (auto it = fdec->param_begin(); it != fdec->param_end(); ++it)
        {    
            ParmVarDecl* parVar=*it;
            QualType qtPar=parVar->getType();
            //When the parameter might lead to modifications (not constified and a pointer or a reference)
            if (!parVar->getType().isConstQualified()&&(isPointerQualType(qtPar)||isReferenceQualType(qtPar)))
            {
                int index=std::distance(fdec->param_begin(),it);
                ValueDecl* curDecl=getInnerPtr(ce->getArg(index));
                unconstifyByPropagation(getHashTableValue(curDecl));               
            } 
        }
    }
    return true;
}


void unconstifyByPropagation(const_arg* varArg)
{
    std::stack<const_arg*> stackUnconst;
    stackUnconst.push(varArg);
    //Will unconst all const dependencies,
    while(!stackUnconst.empty())
    {
        const_arg* curArg=stackUnconst.top();
        stackUnconst.pop();
        if(curArg!=NULL)
        {
            curArg->is_const=false;
            for (std::vector<const_arg*>::iterator it = curArg->dependencies.begin(); it != curArg->dependencies.end(); ++it)
            {
                //If it is not const, then it has already been unconstified, so no need to add it to the stack
                if((*it)->is_const)
                    stackUnconst.push(*it);
            }
        }
        
    }
}
