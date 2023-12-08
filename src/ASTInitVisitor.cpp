#include "../include/ASTInitVisitor.hpp"

using namespace clang;

//To avoid crashes
bool ASTInitVisitor::VisitStmt(Stmt *s)
{
    return true;
}
bool ASTInitVisitor::VisitVarDecl(VarDecl *v)
{
    //Initialize the const_arg for any variable
    const_arg &curDeclArg = const_arg_table[getHashKey(v)];
    curDeclArg.is_const = true;
    curDeclArg.declaration = v;
    
    //Further the initialization, for pointers and references (for now)
    const Type *intype = v->getType().getTypePtrOrNull();
    if (intype != NULL)
    {           

        if (intype->isPointerType()||intype->isReferenceType())
        {
            ValueDecl* initDecl;
            if (intype->isPointerType())
                initDecl=getInnerPtr(v->getInit());
            else
                initDecl=getInnerDecl(v->getInit()) ;
            curDeclArg.is_ptr_or_ref = true;
            if (initDecl != NULL){
                //If the pointer is unconst, then the values referenced by it have to be unconst too
                curDeclArg.dependencies.push_back(
                    &(const_arg_table[getHashKey(initDecl)])
                    );
                //The value is referenced, so if it's unconst, we have to unconst what refers to it
                const_arg_table[getHashKey(initDecl)].dependencies.push_back(&curDeclArg);              
            }
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
                ValueDecl* curDecl=getInnerDecl(ce->getArg(index));                   
                curArg.dependencies.push_back(&(
                    const_arg_table[getHashKey(curDecl)])
                    );
            }
        }
    }
    return true;
}
