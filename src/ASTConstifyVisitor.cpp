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
            std::string qualifiedLeftName = leftSide->getDecl()->getQualifiedNameAsString();
            const_arg *curArg = &(const_arg_table[qualifiedLeftName]);
            curArg->is_const = false;
            QualType t = curArg->declaration->getType();
            t.removeLocalConst();
            curArg->declaration->setType(t);
            int i = 0;
            while (curArg->dependencies != NULL && i < 100)

            {
                i++;
                curArg = curArg->dependencies;
                curArg->is_const = false;
                QualType t = curArg->declaration->getType();
                t.removeLocalConst();
                curArg->declaration->setType(t);
            }
        }
        return true;
    }