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
        const_arg &curDeclArg = const_arg_table[getHashKey(v)];
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
                if (dr != NULL){
                    //If we unconst curDeclArg, we have to unconst pointed value
                    curDeclArg.dependencies.push_back(
                        &(const_arg_table[getHashKey(dr->getDecl())])
                        );
                    //The value is referenced, so if it's unconst, we have to unconst its reference
                    const_arg_table[getHashKey(dr->getDecl())].dependencies.push_back(&curDeclArg);
                }
                // std::stringstream SSprint;
                // SSprint<<"\n"<<temp->getQualifiedNameAsString()<<"\n";
                // TheRewriter.InsertText(v->getEndLoc(),SSprint.str(),true,true);
                // curDeclArg.dependencies=&const_arg_table[dependence];
                // llvm::outs() << temp->getQualifiedNameAsString()<<"\n";
            }
        }
        return true;
    }
    bool ASTInitVisitor::VisitCallExpr(CallExpr *ce)
    {
        FunctionDecl* fdec;
        if( (fdec =ce->getDirectCallee()) !=NULL)
        {
            for (auto it = fdec->param_begin(); it != fdec->param_end(); ++it)
            {    
                ParmVarDecl* parVar=*it;
                const_arg& curArg=const_arg_table[getHashKey(parVar)]; 
                if (curArg.is_ptr_or_ref)
                {
                    int index=std::distance(fdec->param_begin(),it);
                    DeclRefExpr* curExpr=cast<DeclRefExpr>(ce->getArg(index)->IgnoreCasts());                   
                    curArg.dependencies.push_back(&(
                        const_arg_table[getHashKey(curExpr->getDecl())])
                        );
                    //llvm::outs()<<"Ici "<<curArg.dependencies.size()<<" "<<curArg.declaration->getQualifiedNameAsString()<<" "<<curArg.dependencies.back()->declaration->getQualifiedNameAsString()<<"\n";
                }
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

