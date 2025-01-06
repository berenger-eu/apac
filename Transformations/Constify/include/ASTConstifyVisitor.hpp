#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTConstifyVisitor : public RecursiveASTVisitor<ASTConstifyVisitor> {
public:
  ASTConstifyVisitor(Rewriter &R, SymTab &SymTableIn)
      : TheRewriter(R), SymT(SymTableIn) {};
  inline bool VisitStmt(Stmt *) { return true; };
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (fDecl->getNameAsString().find("_apacSeq") == std::string::npos) {
      return RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  bool VisitCXXMethodDecl(CXXMethodDecl *);
  bool VisitBinaryOperator(BinaryOperator *);
  bool VisitUnaryOperator(UnaryOperator *);
  bool VisitReturnStmt(ReturnStmt *);
  // bool VisitCXXMemberCallExpr(CXXMemberCallExpr* );
  bool VisitCallExpr(CallExpr *);
  bool VisitVarDecl(VarDecl *);

private:
  Rewriter &TheRewriter;
  SymTab &SymT;
};
void unconstifyByPropagation(const_arg *);
