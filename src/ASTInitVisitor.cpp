#include "../include/ASTInitVisitor.hpp"





using namespace clang;

//To avoid crashes
bool ASTInitVisitor::VisitStmt(Stmt *s)
{
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
            ValueDecl* initDecl=NULL;
            if (intype->isPointerType())
                initDecl=getInnerPtr(v->getInit());
            else
                initDecl=getInnerDecl(v->getInit()) ;
            assert(initDecl!=NULL);
            const_arg* initDeclArg=getHashTableValue(initDecl);
            if (initDecl != NULL){
                //If the pointer is unconst, then the values referenced by it have to be unconst too
                addDependencyHashTable(curDeclArg,initDeclArg);
                //The value is referenced, so if it's unconst, we have to unconst what refers to it
                addDependencyHashTable(initDeclArg,curDeclArg);
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
        for (auto it = fdec->param_begin(); it != fdec->param_end(); ++it)
        {    
            ParmVarDecl* parVar=*it;
            const_arg* curPar=getHashTableValue(parVar); 
            //Adds the dependency if the parameter is a Pointer or a reference
            if (curPar->is_ptr_or_ref)
            {
                int index=std::distance(fdec->param_begin(),it);
                ValueDecl* curDecl;
                curDecl=getInnerPtr(ce->getArg(index));
                if(curDecl!=NULL)
                    addDependencyHashTable(curPar,getHashTableValue(curDecl));
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
            ValueDecl* leftSideDecl=getInnerPtr(bop->getLHS());
            const_arg *curArg = getHashTableValue(leftSideDecl);
            const_arg *pointedArg = getHashTableValue(getInnerPtr(bop->getRHS()));
            //The pointer and the pointee depend on each other
            curArg->dependencies.push_back(pointedArg);
            pointedArg->dependencies.push_back(curArg);

        }
    }   
    return true;
}
