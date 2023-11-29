#include "core.hpp"
extern std::unordered_map<std::string, struct const_arg> const_arg_table;
using namespace clang;
class ASTConstifyVisitor : public RecursiveASTVisitor<ASTConstifyVisitor>
{
public:
    ASTConstifyVisitor(Rewriter &R) : TheRewriter(R) {};
    bool VisitStmt(Stmt *);
    bool VisitBinaryOperator(BinaryOperator*);
    bool VisitUnaryOperator(UnaryOperator*);
private:
    Rewriter &TheRewriter;
};
void unconstifyByPropagation(const_arg*);
