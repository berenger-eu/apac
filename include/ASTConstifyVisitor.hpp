#include "core.hpp"
#include "SymTab.hpp"
using namespace clang;
class ASTConstifyVisitor : public RecursiveASTVisitor<ASTConstifyVisitor>
{
public:
    ASTConstifyVisitor(Rewriter &R,SymTab& SymTableIn) : TheRewriter(R),SymT(SymTableIn) {};
    bool VisitStmt(Stmt *);
    bool VisitCXXMethodDecl(CXXMethodDecl*);
    bool VisitBinaryOperator(BinaryOperator*);
    bool VisitUnaryOperator(UnaryOperator*);
    bool VisitReturnStmt(ReturnStmt*);
    //bool VisitCXXMemberCallExpr(CXXMemberCallExpr* );
    bool VisitCallExpr(CallExpr* );
private:
    Rewriter &TheRewriter;
    SymTab& SymT;
};
void unconstifyByPropagation(const_arg*);
