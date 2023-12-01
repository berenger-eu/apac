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
   bool ASTPrintVisitor::VisitParmVarDecl(ParmVarDecl* pvd)
   {
    rewriteSingleDecl(pvd);
    return true;
   }
   bool ASTPrintVisitor::VisitDeclStmt(DeclStmt* declStatement)
   {
        
        std::stringstream SSprint;
        if((declStatement->isSingleDecl()))
        {
            Decl* d=declStatement->getSingleDecl();
            if(isa<VarDecl>(d))
            {
                rewriteSingleDecl(cast<VarDecl>(d));
            }    

        }
        //Multiple decl
        else
	    {
            TheRewriter.RemoveText(SourceRange(declStatement->getBeginLoc(),declStatement->getEndLoc()));
            DeclGroup& dgr=declStatement->getDeclGroup().getDeclGroup();
            int dgrSize=dgr.size();
            for(int i=0;i<dgrSize;i++)
            {
                
                if(isa<VarDecl>(dgr[i]))
                {
                    VarDecl* vd =cast<VarDecl>(dgr[i]);
                    PrepareRewriteVarDecl(vd,SSprint);
                    if(i<dgrSize-1)
                        SSprint<<CHAR_NEWLINE;
                }
            }
            TheRewriter.InsertTextAfter(declStatement->getBeginLoc(),SSprint.str());
        }
        return true;
   }
    void ASTPrintVisitor::PrepareRewriteVarDecl(VarDecl *v,std::stringstream& stream)
    {
        QualType qt = v->getType();
        const Type *intype = qt.getTypePtrOrNull();
        //Here we modify the type of the variables to add const if needed
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
        //We prepare the change in the text ( Type Var)
        stream<<v->getType().getAsString()<<' '<<v->getNameAsString();
        //If the variable is initialized, we add the initializer too
        if(v->getInit()!=NULL){
            PrintingPolicy print_policy(v->getASTContext().getLangOpts());
            std::string initString;
            llvm::raw_string_ostream stringStreamInit(initString);
            v->getInit()->printPretty(stringStreamInit,NULL,print_policy);
            stream<<CHAR_ASSIGNEMENT<<initString;
        }
        stream<<CHAR_INSTR_END;
    }
void    ASTPrintVisitor::rewriteSingleDecl(VarDecl* vd)
{
    if(const_arg_table[getHashKey(vd)].is_const)
    {
        addConstToVar(vd);
        TheRewriter.ReplaceText(SourceRange(vd->getTypeSpecStartLoc(),vd->getTypeSpecEndLoc()),vd->getType().getAsString());
    }
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