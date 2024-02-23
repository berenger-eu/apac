#include "core.hpp"
#include "SymTab.hpp"
using namespace clang;
class ASTInitVisitor : public RecursiveASTVisitor<ASTInitVisitor>
{
public:
    ASTInitVisitor(Rewriter &R,SymTab& SymTableIn) : TheRewriter(R),SymT(SymTableIn) {};
    bool VisitStmt(Stmt *);
    bool VisitCXXThisExpr(CXXThisExpr* );
    bool VisitCXXMethodDecl(CXXMethodDecl* );
    bool VisitFieldDecl(FieldDecl *);
    bool VisitVarDecl(VarDecl *);
    bool VisitCallExpr(CallExpr *);
    bool VisitBinaryOperator(BinaryOperator*);
    bool VisitCXXRecordDecl(CXXRecordDecl*);
private:
    Rewriter &TheRewriter;
    SymTab& SymT;
};
