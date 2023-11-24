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
            DeclRefExpr *leftSide; //,rightSide;
            leftSide = cast<DeclRefExpr>(bop->getLHS());
            // rightSide=cast<DeclRefExpr>(bop->getRHS());
             
            const_arg *curArg = &(const_arg_table[getHashKey(leftSide->getDecl())]);
            
            unconstifyByPropagation(curArg);
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
        curArg->is_const=false;
        stackUnconst.pop();
        if(curArg!=NULL)
        {
            for (std::vector<const_arg*>::iterator it = curArg->dependencies.begin(); it != curArg->dependencies.end(); ++it)
            {
                stackUnconst.push(*it);
            }
        }
        
    }
}
