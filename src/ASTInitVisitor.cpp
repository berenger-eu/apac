#include "../include/ASTInitVisitor.hpp"

using namespace clang;
// To initializa the hash table (and analyze dependencies, to be moved to different pass)

    bool ASTInitVisitor::VisitStmt(Stmt *s)
    {
        return true;
    }
    /*		bool VisitFunctionDecl(FunctionDecl *f) {
            // Only function definitions (with bodies), not declarations.
                    for(FunctionDecl::param_iterator b=f->param_begin(),e=f->param_end();b!=e;++b)
                    {
                            ParmVarDecl* pv=*b;
                            pv->setType(pv->getType().withConst());
                    }
                    return true;
            }
    */
    bool ASTInitVisitor::VisitVarDecl(VarDecl *v)
    {

        Expr *expr = v->getInit();
        DeclRefExpr *dr = NULL;
        if (expr != NULL && isa<DeclRefExpr>(expr->IgnoreCasts()))
            dr = cast<DeclRefExpr>(expr->IgnoreCasts());
        const_arg &curDeclArg = const_arg_table[v->getQualifiedNameAsString()];
        curDeclArg.is_const = true;
        curDeclArg.declaration = v;
        QualType qt = v->getType();
        const Type *intype = qt.getTypePtrOrNull();
        if (intype != NULL)
        {

            if (intype->isPointerType())
            {
                curDeclArg.is_ptr_or_ref = true;
            }
            else if (intype->isReferenceType())
            {

                curDeclArg.is_ptr_or_ref = true;
                if (dr != NULL)
                    curDeclArg.dependencies = &(const_arg_table[cast<NamedDecl>(dr->getDecl())->getQualifiedNameAsString()]);
                // std::stringstream SSprint;
                // SSprint<<"\n"<<temp->getQualifiedNameAsString()<<"\n";
                // TheRewriter.InsertText(v->getEndLoc(),SSprint.str(),true,true);
                // curDeclArg.dependencies=&const_arg_table[dependence];
                // llvm::outs() << temp->getQualifiedNameAsString()<<"\n";
            }
        }
        return true;
    }
    bool ASTInitVisitor::VisitBinaryOperator(BinaryOperator *bop)
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
    /*	bool VisitDeclRefExpr(DeclRefExpr* de)
        {
            VarDecl* v=dyn_cast<VarDecl>(de->getDecl());
            if(v)
            {
            }
            return true;
        }*/

