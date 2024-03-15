#include "../include/ASTPrintVisitor.hpp"
using namespace clang;
// Printing pass, rewrite variables and add const if they are

    // To avoid errors on unused Stmt, crashes without it
bool ASTPrintVisitor::VisitStmt(Stmt *s)
{
    return true;
}

bool ASTPrintVisitor::VisitParmVarDecl(ParmVarDecl* pvd)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(pvd->getBeginLoc()))
        return true;
    rewriteSingleDecl(pvd);
    return true;
}
bool ASTPrintVisitor::VisitCXXMethodDecl(CXXMethodDecl* metDecl)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(metDecl->getBeginLoc()))
        return true;
    metDecl->setType(metDecl->getType().withConst());
    const_arg* metArg=SymT.getInnerConstArg(metDecl);
    if(metArg->is_const&&!metDecl->isConst())
    {
        if(!metDecl->isThisDeclarationADefinition())
            TheRewriter.InsertTextAfterToken(metDecl->getEndLoc()," const ");
            //Prints const at the end of a method declaration (not definition)
        else
            TheRewriter.InsertTextAfter(metDecl->getBody()->getBeginLoc()," const ");  
            //Prints const in the definition of a method
    }
    return true;
}
bool ASTPrintVisitor::VisitDeclStmt(DeclStmt* declStatement)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(declStatement->getBeginLoc()))
        return true;  
    
    std::stringstream SSprint;
    if((declStatement->isSingleDecl()))
    {
        Decl* d=declStatement->getSingleDecl();
        if(isa<VarDecl>(d))
            rewriteSingleDecl(cast<VarDecl>(d));    
    }
    //Multiple decl
    else
    {
        DeclGroup& dgr=declStatement->getDeclGroup().getDeclGroup();
        int dgrSize=dgr.size();
        //Remove old multiple declaration to rewrite multiple single declarations
        TheRewriter.RemoveText(SourceRange(declStatement->getBeginLoc(),declStatement->getEndLoc()));
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
    //Here we modify the type of the variables to add const if needed
    if (SymT.getHashTableValue(v)->is_const)
            addConstToVar(v);
    //We prepare the change in the text ( Type VarName)
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
    //We rewrite the declaration if the variable is const and there is an initilisation ( "const int a;" is not valid)
    if(SymT.getHashTableValue(vd)->is_const&&vd->getInit()!=NULL)
    {
        addConstToVar(vd);
        //Add a space char, otherwise int*a (valid) can become int*consta instead of int*const a 
        std::string result=vd->getType().getAsString()+" ";
        TheRewriter.ReplaceText(SourceRange(vd->getBeginLoc(),vd->getTypeSpecEndLoc()),result);
    }
}


void    ASTPrintVisitor::rewriteSingleDecl(ParmVarDecl* vd)
{
    //We rewrite the declaration if the parameter is const
    if(SymT.getHashTableValue(vd)->is_const)
    {
        addConstToVar(vd);
        //Add a space char, otherwise int*a (valid) can become int*consta instead of int*const a 
        std::string result=vd->getType().getAsString()+" ";
        TheRewriter.ReplaceText(SourceRange(vd->getBeginLoc(),vd->getTypeSpecEndLoc()),result);
    }
}
//We directly modify the AST, this makes printing the new type easier
void addConstToVar(ValueDecl* valD)
{
    QualType newQualType=addConstToQualType(valD->getType(),valD->getASTContext());
    valD->setType(newQualType);
}

QualType addConstToQualType(QualType qt,ASTContext& aContext)
{
    const Type* innerType= qt.getTypePtrOrNull();
    if(innerType==NULL)
        ;
    else if (innerType->isBuiltinType())
    {
        qt=addConstToBuiltInType(qt,aContext);
    }
    else if (innerType->isReferenceType())
    {
        qt=addConstToReferenceType(qt,aContext);    
    }
    else if (innerType->isPointerType())
    {
        qt=addConstToPointerType(qt,aContext);
    }
    else if (innerType->isConstantArrayType())
    {
        qt=addConstToBuiltInType(qt,aContext);
    }
    else
    {
        qt.addConst();
        llvm::outs()<<"Adding const to the following type : "<<qt.getAsString()<<" might not work\n";
        qt->dump();
        innerType->dump();
    }
    return qt;
}   

QualType addConstToBuiltInType(QualType qt,ASTContext& aContext){
    return qt.withConst();
}
QualType addConstToReferenceType(QualType qt,ASTContext& aContext)
{
    QualType refInnerType=addConstToQualType(qt.getNonReferenceType(),aContext);
    return aContext.getLValueReferenceType(refInnerType);
}
QualType addConstToPointerType(QualType qt,ASTContext& aContext)
{
    int degree=0;
    const Type* tempType=qt.getTypePtrOrNull();
    //We look for the pointed type and the degree of the pointer
    while(tempType!=NULL&&tempType->isPointerType())
    {
        degree++;
        qt=tempType->getPointeeType();
        tempType=qt.getTypePtrOrNull();
    }
    QualType finalType=qt;
    for (int i=0;i<degree;i++)
    {
        finalType.addConst();
        finalType=aContext.getPointerType(finalType);    
    }
    finalType.addConst();
    return finalType;
}