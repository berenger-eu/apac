#include "core.hpp"
using namespace clang;
class ASTInitVisitor : public RecursiveASTVisitor<ASTInitVisitor>
{
public:
    ASTInitVisitor(Rewriter &R) : TheRewriter(R) {};
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
};
