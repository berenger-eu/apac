#include "core.hpp"
using namespace clang;
class ASTConstifyVisitor : public RecursiveASTVisitor<ASTConstifyVisitor>
{
public:
    ASTConstifyVisitor(Rewriter &R) : TheRewriter(R) {};
    bool VisitStmt(Stmt *);
    bool VisitBinaryOperator(BinaryOperator*);
    bool VisitUnaryOperator(UnaryOperator*);
    bool VisitReturnStmt(ReturnStmt*);
    //bool VisitCXXMemberCallExpr(CXXMemberCallExpr* );
    bool VisitCallExpr(CallExpr* );
private:
    Rewriter &TheRewriter;
};
void unconstifyByPropagation(const_arg*);
