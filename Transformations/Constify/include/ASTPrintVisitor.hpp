#include "core.hpp"
#include "SymTab.hpp"
using namespace clang;
class ASTPrintVisitor : public RecursiveASTVisitor<ASTPrintVisitor>
{
public:
    ASTPrintVisitor(Rewriter &R,SymTab& SymTableIn) : TheRewriter(R),SymT(SymTableIn) {}
    // To avoid errors on unused Stmt
    inline bool VisitStmt(Stmt *){return true ;};
    //bool VisitFunctionDecl(FunctionDecl *);
    bool VisitDeclStmt(DeclStmt* );
    bool VisitParmVarDecl(ParmVarDecl*);
    bool VisitCXXMethodDecl(CXXMethodDecl*);
    void PrepareRewriteVarDecl(VarDecl *,std::stringstream&);
    void rewriteSingleDecl(VarDecl* );
    void rewriteSingleDecl(ParmVarDecl* );


private:
   Rewriter &TheRewriter;
   SymTab& SymT;
};

void addConstToVar(ValueDecl*);
QualType addConstToQualType(QualType,ASTContext& ); 
QualType addConstToBuiltInType(QualType,ASTContext& );
QualType addConstToReferenceType(QualType,ASTContext& );
QualType addConstToPointerType(QualType,ASTContext& );