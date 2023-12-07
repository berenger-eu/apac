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
        const_arg &curDeclArg = const_arg_table[getHashKey(v)];
        curDeclArg.is_const = true;
        curDeclArg.declaration = v;
        const Type *intype = v->getType().getTypePtrOrNull();
        if (intype != NULL)
        {
            
            if (intype->isPointerType())
            {
                ValueDecl *initDecl=getInnerDecl(v->getInit()) ;
                curDeclArg.is_ptr_or_ref = true;
                if (initDecl != NULL){
                    //If the pointer is unconst, then the values referenced by it have to be unconst too
                    curDeclArg.dependencies.push_back(
                        &(const_arg_table[getHashKey(initDecl)])
                        );
                    //The value is referenced, so if it's unconst, we have to unconst what refers to it
                    const_arg_table[getHashKey(initDecl)].dependencies.push_back(&curDeclArg);
                     llvm::outs()<<curDeclArg.declaration->getNameAsString()<<'\t'<<curDeclArg.dependencies[0]->declaration->getNameAsString()<<'\n';
              
                }
            }
            else if (intype->isReferenceType())
            {
                ValueDecl *initDecl=getInnerDecl(v->getInit()) ;
                curDeclArg.is_ptr_or_ref = true;
                if (initDecl != NULL){
                    //If we unconst curDeclArg, we have to unconst pointed value
                    curDeclArg.dependencies.push_back(
                        &(const_arg_table[getHashKey(initDecl)])
                        );
                    //The value is referenced, so if it's unconst, we have to unconst what refers to it
                    const_arg_table[getHashKey(initDecl)].dependencies.push_back(&curDeclArg);
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

