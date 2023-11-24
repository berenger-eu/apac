#include "../include/ASTPrintVisitor.hpp"
using namespace clang;
// Printing pass, rewrite variables and add const if they are

    // To avoid errors on unused Stmt
    bool ASTPrintVisitor::VisitStmt(Stmt *s)
    {
        return true;
    }
/*
    bool ASTPrintVisitor::VisitFunctionDecl(FunctionDecl *f)
    {
        // Only function definitions (with bodies), not declarations.
        for (FunctionDecl::param_iterator b = f->param_begin(), e = f->param_end(); b != e; ++b)
        {
            ParmVarDecl *pv = *b;

            QualType qt = pv->getType();
            const Type *intype = qt.getTypePtrOrNull();

            if (intype != NULL&&const_arg_table[pv->getQualifiedNameAsString()].is_const)

            {
                if (intype->isBuiltinType())
                {
                    addConstToVar(pv);
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
                    addConstToVar(pv);
                    
                }
            }
        }
        return true;
    }
    */
   
    bool ASTPrintVisitor::VisitVarDecl(VarDecl *v)
    {
        QualType qt = v->getType();
        const Type *intype = qt.getTypePtrOrNull();
        if (const_arg_table[getHashKey(v)].is_const&&intype != NULL)

            {
                
                if (intype->isBuiltinType())
                {
                    addConstToVar(v);
                }

                else if (intype->isPointerType())
                {
                    QualType inpv = intype->getPointeeType();
                    inpv.addConst();
                }
                else if (intype->isReferenceType())
                {
                    addConstToVar(v);
                    
                }
            }
        TheRewriter.ReplaceText(SourceRange(v->getTypeSpecStartLoc(), v->getTypeSpecEndLoc()), v->getType().getAsString());
        return true;
    }
//On modifie l'AST pour faciliter l'affichage des variables modifiés
void addConstToVar(ValueDecl* valD)
{
    QualType tfirst=valD->getType();
    const Type* innerType= tfirst.getTypePtrOrNull();
    if(innerType==NULL)
        return;
    if (innerType->isBuiltinType())
    {
        valD->setType(tfirst.withConst());
    }
    else if (innerType->isReferenceType())
    {
        QualType refInnerType=tfirst.getNonReferenceType();
        refInnerType.addConst();
        ASTContext& aContext=valD->getASTContext();
        valD->setType(aContext.getLValueReferenceType(refInnerType));
    }

}