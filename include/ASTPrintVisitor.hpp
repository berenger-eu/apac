#include "core.hpp"
using namespace clang;
class ASTPrintVisitor : public RecursiveASTVisitor<ASTPrintVisitor>
{
public:
    ASTPrintVisitor(Rewriter &R) : TheRewriter(R) {}
    // To avoid errors on unused Stmt
    bool VisitStmt(Stmt *);
    //bool VisitFunctionDecl(FunctionDecl *);
    bool VisitDeclStmt(DeclStmt* );
    bool VisitParmVarDecl(ParmVarDecl*);
    void PrepareRewriteVarDecl(VarDecl *,std::stringstream&);
    void rewriteSingleDecl(VarDecl* );
    void rewriteSingleDecl(ParmVarDecl* );

private:
   Rewriter &TheRewriter;
};

void addConstToVar(ValueDecl*);
QualType addConstToQualType(QualType,ASTContext& ); 
QualType addConstToBuiltInType(QualType,ASTContext& );
QualType addConstToReferenceType(QualType,ASTContext& );
QualType addConstToPointerType(QualType,ASTContext& );