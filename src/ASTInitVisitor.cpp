#include "../include/ASTInitVisitor.hpp"





using namespace clang;

//To avoid crashes
bool ASTInitVisitor::VisitStmt(Stmt *s)
{
    return true;
}
bool ASTInitVisitor::VisitCXXThisExpr(CXXThisExpr* thisExpr)
{
    const_arg* curThisArg=getHashTableValue(thisExpr);
    const_arg* curMethArg=getInnerConstArg(lastMethDecl);
    curThisArg->is_const=true;
    curThisArg->is_ptr_or_ref=true;
    addDependencyHashTable(curThisArg,curMethArg);
    return true;
}
bool ASTInitVisitor::VisitCXXMethodDecl(CXXMethodDecl* methDecl)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(methDecl->getBeginLoc()))
        return true;
    //TODO: do it in all calls (has to be done at least once per file)
    //Adds a FileID to the table
    FileID fid=TheRewriter.getSourceMgr().getFileID(methDecl->getLocation());
    fileID_table[fid.getHashValue()]=fid;
    const_arg *curDeclArg = getHashTableValue(methDecl);
    curDeclArg->method = methDecl;
    curDeclArg->is_const=true;
    lastMethDecl=methDecl;
    return true;
}
bool ASTInitVisitor::VisitFieldDecl(FieldDecl *fieldDec)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(fieldDec->getBeginLoc()))
        return true;
    //TODO: do it in all calls (has to be done at least once per file)
    //Adds a FileID to the table
    FileID fid=TheRewriter.getSourceMgr().getFileID(fieldDec->getLocation());
    fileID_table[fid.getHashValue()]=fid;

    //Initialize the const_arg for any variable
    const_arg *curDeclArg = getHashTableValue(fieldDec);
    curDeclArg->field = fieldDec;
    const Type *intype = fieldDec->getType().getTypePtrOrNull();
    if ( intype->isPointerType()||intype->isReferenceType())
        curDeclArg->is_ptr_or_ref = true;
    return true;

}

bool ASTInitVisitor::VisitVarDecl(VarDecl *v)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(v->getBeginLoc()))
        return true;
    //TODO: do it in all calls (has to be done at least once per file)
    //Adds a FileID to the table
    FileID fid=TheRewriter.getSourceMgr().getFileID(v->getLocation());
    fileID_table[fid.getHashValue()]=fid;


    //Initialize the const_arg for any variable
    const_arg *curDeclArg = getHashTableValue(v);
    curDeclArg->is_const = true;
    curDeclArg->declaration = v;
    //Further the initialization, for pointers and references (for now)
    const Type *intype = v->getType().getTypePtrOrNull();
    if (intype != NULL)
    {           

        if ( (intype->isPointerType()||intype->isReferenceType()) && valueInit(v))
        {
            curDeclArg->is_ptr_or_ref = true;
            if(v->getInit()!=NULL)
            {
                const_arg* initDeclArg=getInnerConstArg(v->getInit());
                if (initDeclArg != NULL){
                    //If the pointer is unconst, then the values referenced by it have to be unconst too
                    addDependencyHashTable(curDeclArg,initDeclArg);
                    //The value is referenced, so if it's unconst, we have to unconst what refers to it
                    addDependencyHashTable(initDeclArg,curDeclArg);
                }
            }
        }
    }
    return true;
}

bool ASTInitVisitor::VisitCallExpr(CallExpr *ce)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(ce->getBeginLoc()))
        return true;

    FunctionDecl* fdec;
    //Call to a function but can't retrieve the function
    assert(ce->getDirectCallee()!=NULL);
    
    //For all functions outside of system headers, adds a dependency between the argument and the parameter when needed
    if( (fdec =ce->getDirectCallee()) !=NULL&&!TheRewriter.getSourceMgr().isInSystemHeader(fdec->getBeginLoc()))
    {  
        if(isa<CXXMemberCallExpr>(ce))
        {
            CXXMemberCallExpr* memberCall=(cast<CXXMemberCallExpr> (ce));
            Expr* objExpr=memberCall->getImplicitObjectArgument();
            const_arg* objDeclArg=getInnerConstArg(objExpr);
            const_arg* methDeclArg=getInnerConstArg(memberCall->getMethodDecl());
            addDependencyHashTable( methDeclArg,objDeclArg);

        }
        for (auto it = fdec->param_begin(); it != fdec->param_end(); ++it)
        {    
            ParmVarDecl* parVar=*it;
            const_arg* curPar=getHashTableValue(parVar); 
            assert(curPar->declaration);
            llvm::outs()<<"\n"<<curPar->declaration->getNameAsString()<<curPar->is_ptr_or_ref<<"\n";
            //Adds the dependency if the parameter is a Pointer or a reference
            if (curPar->is_ptr_or_ref)
            {
                int index=std::distance(fdec->param_begin(),it);            
                const_arg* curArg=getInnerConstArg(ce->getArg(index));
                if(curArg!=NULL)
                    addDependencyHashTable(curPar,curArg);
            }
        }
    }
    return true;
}

bool ASTInitVisitor::VisitBinaryOperator(BinaryOperator* bop)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(bop->getBeginLoc()))
        return true;
    //Assignement (outside of declarations) might add a dependency between a value and a pointer
    if(bop->isAssignmentOp())
    {
        if(isPointerQualType(bop->getLHS()->getType()))
        {
            const_arg *curArg = getInnerConstArg(bop->getLHS());
            const_arg *pointedArg = getInnerConstArg(bop->getRHS());
            //The pointer and the pointee depend on each other
            addDependencyHashTable(curArg,pointedArg);
            addDependencyHashTable(pointedArg,curArg);
        }
    }   
    return true;
}
bool ASTInitVisitor::VisitCXXRecordDecl(CXXRecordDecl* recDecl)
{
    return true;
}