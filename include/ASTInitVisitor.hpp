#include "core.hpp"
extern std::unordered_map<std::string, struct const_arg> const_arg_table;
using namespace clang;
class ASTInitVisitor : public RecursiveASTVisitor<ASTInitVisitor>
{
public:
    ASTInitVisitor(Rewriter &R) : TheRewriter(R) {};
    bool VisitStmt(Stmt *);
    bool VisitVarDecl(VarDecl *);
    bool VisitCallExpr(CallExpr *);
private:
    Rewriter &TheRewriter;
};
