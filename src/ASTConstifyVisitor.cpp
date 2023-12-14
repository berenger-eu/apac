#include "../include/ASTConstifyVisitor.hpp"  
using namespace clang;
bool ASTConstifyVisitor::VisitStmt(Stmt *s)
    {
        return true;
    }

bool ASTConstifyVisitor::VisitBinaryOperator(BinaryOperator *bop)
    {
        if (bop->isAssignmentOp())
        {
            ValueDecl* leftSideDecl=getInnerDecl(bop->getLHS());
            // rightSide=cast<DeclRefExpr>(bop->getRHS());
            const_arg *curArg = getHashTableValue(leftSideDecl);
            unconstifyByPropagation(curArg);
        }
        return true;
    }
bool ASTConstifyVisitor::VisitUnaryOperator(UnaryOperator* uop)
{
    if(uop->isIncrementDecrementOp())
    {   
        ValueDecl* innerDecl=getInnerDecl(uop->getSubExpr());
        unconstifyByPropagation(getHashTableValue(innerDecl));
    }
    return true;
}
bool ASTConstifyVisitor::VisitReturnStmt(ReturnStmt* retStmt)
{
    Expr* retValue=retStmt->getRetValue();
    if(isPointerQualType(retValue->getType()))
    {
        ValueDecl* retValDecl=getInnerPtr(retValue);
        unconstifyByPropagation(getHashTableValue(retValDecl));
    }
    //We cast it to its decl because the QualType of retValue for a reference will be int
    else if(isa<DeclRefExpr>(retValue))
    {
        ValueDecl* retDecl=cast<DeclRefExpr>(retValue)->getDecl();
        if(isReferenceQualType(retDecl->getType()))
        {
            ValueDecl* retValDecl=getInnerDecl(retValue);
            unconstifyByPropagation(getHashTableValue(retValDecl));
        }
    }
    return true;
}

void unconstifyByPropagation(const_arg* varArg)
{
    std::stack<const_arg*> stackUnconst;
    stackUnconst.push(varArg);
    while(!stackUnconst.empty())
    {
        const_arg* curArg=stackUnconst.top();
        stackUnconst.pop();
        if(curArg!=NULL)
        {
            curArg->is_const=false;
            for (std::vector<const_arg*>::iterator it = curArg->dependencies.begin(); it != curArg->dependencies.end(); ++it)
            {
                if((*it)->is_const)
                    stackUnconst.push(*it);
            }
        }
        
    }
}
