#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTConstifyVisitor : public APACRecursiveASTVisitor<ASTConstifyVisitor> {
public:
  ASTConstifyVisitor(Rewriter &R, std::string &mainName,
                     std::vector<std::string> &functionsRef,
                     std::vector<std::string> &functionsToIgnoreRef,
                     SymTab &SymTableIn)
      : APACRecursiveASTVisitor(R, mainName, functionsRef,
                                functionsToIgnoreRef),
        SymT(SymTableIn) {}

  bool VisitCXXMethodDecl(CXXMethodDecl *);
  bool VisitBinaryOperator(BinaryOperator *);
  bool VisitUnaryOperator(UnaryOperator *);
  bool VisitReturnStmt(ReturnStmt *);
  // bool VisitCXXMemberCallExpr(CXXMemberCallExpr* );
  bool VisitCallExpr(CallExpr *);
  bool VisitVarDecl(VarDecl *);

private:
  SymTab &SymT;
};
void unconstifyByPropagation(const_arg *);
