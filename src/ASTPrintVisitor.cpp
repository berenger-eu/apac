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
    if (const_arg_table[getHashKey(v)].is_const)
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
    if(const_arg_table[getHashKey(vd)].is_const)
    {
        addConstToVar(vd);
        //Add a space char, otherwise int*a (valid) can become int*consta instead of int*const a 
        std::string result=vd->getType().getAsString()+" ";
        TheRewriter.ReplaceText(SourceRange(vd->getTypeSpecStartLoc(),vd->getTypeSpecEndLoc()),result);
    }
}


//We directly modify the AST, this makes printing the new type easier
void addConstToVar(ValueDecl* valD)
{
    QualType tfirst=valD->getType();
    const Type* innerType= tfirst.getTypePtrOrNull();
    if(innerType==NULL)
        return;
    if (innerType->isBuiltinType())
    {
        addConstToBuiltInType(valD);
    }
    else if (innerType->isReferenceType())
    {
        addConstToReference(valD);    
    }
    else if (innerType->isPointerType())
    {
        addConstToPointer(valD);
    }
}


void addConstToBuiltInType(ValueDecl* valD){
    valD->setType(valD->getType().withConst());
}
void addConstToReference(ValueDecl* valD)
{
    QualType refInnerType=valD->getType().getNonReferenceType();
    refInnerType.addConst();
    ASTContext& aContext=valD->getASTContext();
    valD->setType(aContext.getLValueReferenceType(refInnerType));
}
void addConstToPointer(ValueDecl* valD)
{
    int degree=0;
        
    QualType innerQualType=valD->getType();
    
    ASTContext& acons= valD->getASTContext();
    const Type* tempType=innerQualType.getTypePtrOrNull();
    //We look for the pointed type and the degree of the pointer
    while(tempType!=NULL&&tempType->isPointerType())
    {
        degree++;
        innerQualType=tempType->getPointeeType();
        tempType=innerQualType.getTypePtrOrNull();
    }
    QualType finalType=innerQualType;
    for (int i=0;i<degree;i++)
    {
        finalType.addConst();
        finalType=acons.getPointerType(finalType);    
    }
    finalType.addConst();
    valD->setType(finalType);
}