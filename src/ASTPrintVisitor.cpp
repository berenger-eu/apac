#include "../include/ASTPrintVisitor.hpp"
using namespace clang;
// Printing pass, rewrite variables and add const if they are

    // To avoid errors on unused Stmt
    bool ASTPrintVisitor::VisitStmt(Stmt *s)
    {
        return true;
    }

    bool ASTPrintVisitor::VisitFunctionDecl(FunctionDecl *f)
    {
        // Only function definitions (with bodies), not declarations.
        for (FunctionDecl::param_iterator b = f->param_begin(), e = f->param_end(); b != e; ++b)
        {
            ParmVarDecl *pv = *b;

            QualType qt = pv->getType();
            const Type *intype = qt.getTypePtrOrNull();

            if (intype != NULL)

            {
                if (intype->isBuiltinType())
                {
                    TheRewriter.ReplaceText(SourceRange(pv->getTypeSpecStartLoc(),
                                                        pv->getTypeSpecEndLoc()),
                                            pv->getType().getAsString());
                }

                else if (intype->isPointerType())
                {
                    QualType inpv = intype->getPointeeType();
                    inpv.addConst();
                }
                else if (intype->isReferenceType())
                {
                    QualType qtIn = qt.getNonReferenceType();
                    qtIn.addConst();
                    ASTContext &acons = pv->getASTContext();
                    pv->setType(acons.getLValueReferenceType(qtIn));
                }
            }
        }
        return true;
    }
    bool ASTPrintVisitor::VisitVarDecl(VarDecl *v)
    {
        TheRewriter.ReplaceText(SourceRange(v->getTypeSpecStartLoc(), v->getTypeSpecEndLoc()), v->getType().getAsString());

        return true;
    }